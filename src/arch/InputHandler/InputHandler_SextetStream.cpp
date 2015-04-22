#include "global.h"
#include "InputHandler_SextetStream.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "RageThreads.h"
#include "RageUtil.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

using namespace std;

// In so many words, ceil(n/6).
#define NUMBER_OF_SEXTETS_FOR_BIT_COUNT(n) (((n) + 5) / 6)

#define FIRST_DEVICE DEVICE_JOY1

#define FIRST_JOY_BUTTON JOY_BUTTON_1
#define LAST_JOY_BUTTON JOY_BUTTON_32
#define COUNT_JOY_BUTTON ((LAST_JOY_BUTTON) - (FIRST_JOY_BUTTON) + 1)

#define FIRST_KEY KEY_OTHER_0
#define LAST_KEY KEY_LAST_OTHER
#define COUNT_KEY ((LAST_KEY) - (FIRST_KEY) + 1)

#define BUTTON_COUNT (COUNT_JOY_BUTTON + COUNT_KEY)

#define DEFAULT_TIMEOUT_MS 1000
#define STATE_BUFFER_SIZE NUMBER_OF_SEXTETS_FOR_BIT_COUNT(BUTTON_COUNT)

typedef RString::value_type Sextet;


inline bool IsValidSextet(Sextet s)
{
	return ((s >= 0x30) && (s <= 0x6F));
}

// Removes a trailing line ending (CRLF, CR, or LF).
// Returns true if any line ending was found and removed.
// Returns false if no line ending was found.
inline bool Chomp(RString& line)
{
	size_t len = line.length();
	size_t newlineSize = 0;
	const Sextet CR = 0xD;
	const Sextet LF = 0xA;

	if(len >= 1) {
		Sextet last = line[len - 1];

		if(last == LF) {
			if(len >= 2 && line[len - 2] == CR) {
				// CRLF
				newlineSize = 2;
			}
			else {
				// LF
				newlineSize = 1;
			}
		}
		else if(last == CR) {
			// CR
			newlineSize = 1;
		}
	}

	if(newlineSize > 0) {
		line.resize(len - newlineSize);
		return true;
	}
	return false;
}

// For mutex naming
inline RString PointerAsHex(void * p)
{
	RString result;
	char buffer[sizeof(void*) * 2 + 1];
	snprintf(buffer, sizeof(buffer), "%x", (unsigned int)p);
	result = buffer;
	return result;
}


namespace
{
	class LineReader
	{
	public:
		LineReader() {}
		virtual ~LineReader() {}

		// Returns true if the stream is open. If this method returns false,
		// the caller may assume that the stream is permanently closed.
		virtual bool IsOpen() = 0;

		// Reads a line from input and assigns it to the provided string
		// object. The newline at the end should be omitted.
		//
		// Returns true if a line was read.
		// Returns false (with no particular value for line) if the line
		// read failed with an error or EOF condition, or if a nonblocking
		// or timeout-blocking read returned with no data, or if the stream
		// is closed. Call IsOpen() to determine whether to continue
		// reading. (The suggested timeout for reads is DEFAULT_TIMEOUT_MS.)
		//
		// Implementations whose underlying streams are essentially
		// unreadable after certain unrecoverable conditions (like EOF or
		// some kinds of error) should call Close() on themselves so that
		// resources are freed and so IsOpen() no longer returns true.
		virtual bool ReadLine(RString& line) = 0;

		// If the underlying stream is closeable, close the stream. Calling
		// this on a stream that is not open must be harmless.
		virtual void Close() {}
	};


	class StdCFileLineReader: public LineReader
	{
		private:
			// The buffer size isn't critical; the RString will simply be
			// extended until the line is done.
			static const size_t BUFFER_SIZE = 64;
			char buffer[BUFFER_SIZE];
		protected:
			std::FILE * file;

		public:
			// Make sure to do std::setbuf(file, NULL) on any handle passed
			// here
			StdCFileLineReader(std::FILE * file)
			{
				LOG->Info("Starting InputHandler_SextetStreamFromFile from open std::FILE");
				this->file = file;
			}

			static StdCFileLineReader * Open(const RString& filename, bool alwaysCreate = true)
			{
				std::FILE * file;

				LOG->Info("Opening std::FILE '%s' for input to InputHandler_SextetStreamFromFile", filename.c_str());
				file = std::fopen(filename.c_str(), "rb");

				if(file == NULL) {
					LOG->Warn("Error opening std::FILE '%s' for input: %s", filename.c_str(), std::strerror(errno));
					return alwaysCreate ? new StdCFileLineReader((FILE*)NULL) : NULL;
				}
				else {
					LOG->Info("File opened");
					// Disable buffering on the file
					std::setbuf(file, NULL);

					return new StdCFileLineReader(file);
				}
			}

			~StdCFileLineReader()
			{
				Close();
			}

			virtual void Close()
			{
				if(file != NULL) {
					std::fclose(file);
					file = NULL;
				}
			}

			virtual bool IsOpen()
			{
				return file != NULL;
			}

			virtual bool ReadLine(RString& line)
			{
				bool atLeastOneSuccessfulRead = false;

				line = "";

				while(IsOpen())
				{
					if(fgets(buffer, BUFFER_SIZE, file) != NULL) {
						// At least one fgets call has returned.
						atLeastOneSuccessfulRead = true;

						line += buffer;

						// If a line was fully read (i.e., it ends with a
						// newline), rather than just partially, we can stop
						// here
						if(Chomp(line)) {
							break;
						}
					}
					else {
						if(std::feof(file)) {
							LOG->Info("Closing std::FILE for input to InputHandler_SextetStreamFromFile (reached EOF)");
							Close();
						}
						else if(std::ferror(file)) {
							LOG->Info("Closing std::FILE for input to InputHandler_SextetStreamFromFile (read error)");
							Close();
						}
						else {
							LOG->Info("Closing std::FILE for input to InputHandler_SextetStreamFromFile (unspecified)");
							Close();
						}
					}
				}

				return atLeastOneSuccessfulRead;
			}
	};
}

namespace
{
	class TakeOneLineReader
	{
		private:
			LineReader * item;
			RageMutex mutex;

		public:
			TakeOneLineReader(LineReader * item) :
				mutex("InputHandler_SextetStream_TakeOneLineReader")
			{
				this->item = item;
				mutex.SetName(RString("InputHandler_SextetStream_TakeOneLineReader(") + PointerAsHex(this) + ")");
			}

			LineReader * Take()
			{
				LineReader * ourItem;
				mutex.Lock();
				ourItem = item;
				item = NULL;
				mutex.Unlock();
				return ourItem;
			}
	};
}

namespace
{
	class Impl
	{
		private:
			InputHandler_SextetStream * handler;
			TakeOneLineReader takeReader;

		protected:
			void ButtonPressed(const DeviceInput& di)
			{
				const DeviceInput * pdi = &di;
				handler->_impl_ext(&pdi);
			}

			uint8_t stateBuffer[STATE_BUFFER_SIZE];
			RageThread inputThread;
			bool continueInputThread;

			inline void clearStateBuffer()
			{
				memset(stateBuffer, 0, STATE_BUFFER_SIZE);
			}

			inline void createThread()
			{
				continueInputThread = true;
				inputThread.SetName("SextetStream input thread");
				inputThread.Create(StartInputThread, this);
			}

			static void disposeReader(LineReader * r)
			{
				if(r != NULL) {
					r->Close();
					delete r;
				}
			}

		public:
			Impl(InputHandler_SextetStream * _this, LineReader * reader) :
				handler(_this), takeReader(reader)
			{
				LOG->Info("Number of button states supported by current InputHandler_SextetStream: %u",
					(unsigned)BUTTON_COUNT);
				continueInputThread = false;

				clearStateBuffer();
				createThread();
			}

			virtual ~Impl()
			{
				if(inputThread.IsCreated()) {
					continueInputThread = false;
					inputThread.Wait();
				}
				disposeReader(takeReader.Take());
			}

			virtual void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut)
			{
				vDevicesOut.push_back(InputDeviceInfo(FIRST_DEVICE, "SextetStream"));
			}

			static int StartInputThread(void * p)
			{
				((Impl*) p)->RunInputThread();
				return 0;
			}

			inline void GetNewState(uint8_t * buffer, RString& line)
			{
				size_t lineLen = line.length();
				size_t i, cursor;
				cursor = 0;
				memset(buffer, 0, STATE_BUFFER_SIZE);

				// Copy from line to buffer until either it is full or we've
				// run out of characters. Characters outside the sextet code
				// range (0x30..0x6F) are skipped.
				for(i = 0; i < lineLen; ++i) {
					char b = line[i];
					if(IsValidSextet(b)) {
						buffer[cursor] = b;
						++cursor;
						if(cursor >= STATE_BUFFER_SIZE) {
							break;
						}
					}
				}
			}

			inline DeviceButton ButtonAtIndex(size_t index)
			{
				if(index < COUNT_JOY_BUTTON) {
					return enum_add2(FIRST_JOY_BUTTON, index);
				}
				else if(index < COUNT_JOY_BUTTON + COUNT_KEY) {
					return enum_add2(FIRST_KEY, index - COUNT_JOY_BUTTON);
				}
				else {
					return DeviceButton_Invalid;
				}
			}

			// Compares new state to existing state, sending the
			// corresponding ButtonPressed() notifications.
			inline void ReactToChanges(const uint8_t * newStateBuffer)
			{
				InputDevice id = InputDevice(FIRST_DEVICE);
				uint8_t changes[STATE_BUFFER_SIZE];
				RageTimer now;

				// XOR to find differences
				for(size_t i = 0; i < STATE_BUFFER_SIZE; ++i) {
					changes[i] = stateBuffer[i] ^ newStateBuffer[i];
				}

				// Report on changes
				for(size_t m = 0; m < STATE_BUFFER_SIZE; ++m) { // m: byte within changes buffer
					for(size_t n = 0; n < 6; ++n) { // n: bit within byte
						size_t bi = (m * 6) + n; // bi: button index
						if(bi < BUTTON_COUNT) {
							if(changes[m] & (1 << n)) { // i.e. if bi's value changed
								bool value = newStateBuffer[m] & (1 << n);
								LOG->Trace("SS button index %zu %s", bi, value ? "pressed" : "released");
								DeviceInput di = DeviceInput(id, ButtonAtIndex(bi), value, now);
								ButtonPressed(di);
							}
						}
					}
				}

				// Update current state
				memcpy(stateBuffer, newStateBuffer, STATE_BUFFER_SIZE);
			}

			void RunInputThread()
			{
				RString line;
				LineReader * reader = takeReader.Take();

				LOG->Trace("Input thread started");

				if(reader == NULL) {
					LOG->Warn("Line reader for SextetStream input is missing or already used");
					return;
				}
				else if(!reader->IsOpen()) {
					LOG->Warn("Line reader for SextetStream input is closed");
				}
				else {
					LOG->Trace("Got line reader");
					while(continueInputThread) {
						LOG->Trace("Reading line");
						if(reader->ReadLine(line)) {
							LOG->Trace("Got line: '%s'", line.c_str());
							if(line.length() > 0) {
								uint8_t newStateBuffer[STATE_BUFFER_SIZE];
								GetNewState(newStateBuffer, line);
								ReactToChanges(newStateBuffer);
							}
						}
						else {
							if(!reader->IsOpen()) {
								// Error or EOF condition.
								LOG->Trace("Reached end of SextetStream input");
								continueInputThread = false;
							}
							// The line read is allowed to return false if a
							// nonblocking read came up empty or if a read
							// operation timed out before a full line
							// arrived. In those cases we just continue the
							// loop.
						}
					}
					LOG->Info("SextetStream input stopped");
				}
				disposeReader(reader);
			}
	};
}

#define IMPL ((Impl*)_impl)

void InputHandler_SextetStream::_impl_ext(void * p)
{
	const DeviceInput ** ppdi = (const DeviceInput **)p;
	ButtonPressed(**ppdi);
}

void InputHandler_SextetStream::GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut)
{
	if(IMPL != NULL) {
		IMPL->GetDevicesAndDescriptions(vDevicesOut);
	}
}

InputHandler_SextetStream::InputHandler_SextetStream()
{
	_impl = NULL;
}

InputHandler_SextetStream::~InputHandler_SextetStream()
{
	if(IMPL != NULL) {
		delete IMPL;
	}
}

// SextetStreamFromFile

REGISTER_INPUT_HANDLER_CLASS (SextetStreamFromFile);

#if defined(_WINDOWS)
	#define DEFAULT_INPUT_FILENAME "\\\\.\\pipe\\StepMania-Input-SextetStream"
#else
	#define DEFAULT_INPUT_FILENAME "Data/StepMania-Input-SextetStream.in"
#endif
static Preference<RString> g_sSextetStreamInputFilename("SextetStreamInputFilename", DEFAULT_INPUT_FILENAME);


InputHandler_SextetStreamFromFile::InputHandler_SextetStreamFromFile(FILE * file)
{
	_impl = new Impl(this, new StdCFileLineReader(file));
}

InputHandler_SextetStreamFromFile::InputHandler_SextetStreamFromFile(const RString& filename)
{
	_impl = new Impl(this, StdCFileLineReader::Open(filename));
}

InputHandler_SextetStreamFromFile::InputHandler_SextetStreamFromFile()
{
	_impl = new Impl(this, StdCFileLineReader::Open(g_sSextetStreamInputFilename));
}

/*
 * Copyright © 2014-2015 Peter S. May
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


