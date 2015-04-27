#include "global.h"
#include "InputHandler_SextetStream.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "RageThreads.h"
#include "RageUtil.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <queue>

using namespace std;
using namespace X_InputHandler_SextetStream;

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


namespace
{
	typedef RString::value_type Sextet;
	typedef RString::value_type RChar;

	inline bool isValidSextet(Sextet s)
	{
		return ((s >= 0x30) && (s <= 0x6F));
	}

	// Removes a trailing line ending (CRLF, CR, or LF).
	// Returns true if any line ending was found and removed.
	// Returns false if no line ending was found.
	inline bool chomp(RString& line)
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
	inline RString pointerAsHex(const void * p)
	{
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%p", p);
		return RString(buffer);
	}

	// Automatically generates a mutex name based on a differentiating string
	// and the address of an object serving as its topic.
	inline RString mutexName(const RString& name, const void * p)
	{
		return RString("InputHandler_SextetStream@") + name + "(" + pointerAsHex(p) + ")";
	}

	const RChar CR = 0x0D;
	const RChar LF = 0x0A;
	const RChar * CRLF = "\x0D\x0A";
}

namespace
{
	// Converts arbitrary char input into lines.
	// Not (meant to be) thread-safe; synchronize accesses as needed.
	class LineBuffer
	{
		private:
			bool sawCr;
			bool beganLine;
			RString * partialLine;
			queue<const RString *> lines;

			// Given a sequence of input characters, finds the first CR or
			// LF and assigns it to *found. The parts of the input to the
			// left and right of that character are assigned to *left and
			// *right, respectively, as newly allocated strings. Then, true
			// is returned.
			//
			// Any of left, right, or found may be NULL. If left or right is
			// NULL, the new string object is not created for that side.
			//
			// If the input string contains no CR or LF, *left, *found, and
			// *right remain unchanged and false is returned.
			static inline bool splitAtLineEnding(const RString& input,
				const RString ** left,
				RChar * found,
				const RString ** right)
			{
				size_t result = input.find_first_of(CRLF);
				if(result == RString::npos) {
					return false;
				}
				else {
					if(left != NULL) {
						*left = new RString(input.substr(0, result));
					}
					if(found != NULL) {
						*found = input[result];
					}
					if(right != NULL) {
						*right = new RString(input.substr(result + 1));
					}
					return true;
				}
			}

			// Append value to line in progress, then delete value.
			inline void appendAndDelete(const RString * value) {
				if(value != NULL) {
					if(partialLine == NULL)
						partialLine = new RString(*value);
					else
						*partialLine += *value;

					delete value;
				}
			}

			// Enqueue value as an output line.
			inline void pushLines(const RString * value) {
				if(value != NULL) {
					lines.push(value);
				}
			}

			// Dequeue an output line.
			inline const RString * shiftLines() {
				if(lines.empty()) {
					return NULL;
				}
				const RString * item = lines.front();
				lines.pop();
				return item;
			}

			// Move the line in progress to the output queue, regardless of
			// whether a line ending was found. (If there is no line in
			// progress, this is a no-op.)
			inline void flush() {
				if(partialLine != NULL) {
					pushLines(partialLine);
					partialLine = NULL;
				}
			}

			// Discard the line in progress, if it exists.
			inline void clearPartialLine() {
				if(partialLine != NULL) {
					delete partialLine;
					partialLine = NULL;
				}
			}

			// Discard all lines in the output queue.
			inline void clearLines() {
				const RString * item;
				while((item = shiftLines()) != NULL) {
					delete item;
				}
			}

			// Buffer a sequence of input characters, moving lines to the
			// lines queue as they are found.
			bool addData(const RString * topic) {
				const RString * left;
				const RString * right;
				RChar found;

				while(splitAtLineEnding(*topic, &left, &found, &right)) {
					delete topic;
					topic = right;

					if(sawCr && left->empty() && found == LF) {
						// Discard LF after CR
						sawCr = false;
						delete left;
					}
					else {
						sawCr = (found == CR);
						appendAndDelete(left);
						flush();
					}
				}

				if(topic->empty()) {
					// This is not appended so blank lines are not counted
					// double. (Blank lines are counted when their line
					// endings are encountered.)
					delete topic;
				}
				else {
					appendAndDelete(topic);
				}

				return HasNext();
			}

		public:
			LineBuffer() :
				sawCr(false),
				partialLine(NULL)
			{
			}

			~LineBuffer() {
				clearPartialLine();
				clearLines();
			}

			// Buffer characters copied from the given string.
			bool AddData(const RString& data) {
				return addData(new RString(data));
			}

			// Buffer characters copied from the given array.
			bool AddData(const RChar * data, size_t length) {
				return addData(new RString(data, length));
			}

			// Put all currently pending data to a line, even if there is no
			// line ending found. Use (for example) to get the last line of a
			// file that has no line ending at EOF.
			//
			// There is no Close() method for this object, but Flush()
			// should usually be called from its owner's Close()-like method
			// where applicable.
			void Flush() {
				flush();
			}

			// Return true if one or more lines are waiting to be output,
			// false otherwise.
			bool HasNext() {
				return !lines.empty();
			}

			// Dequeue and return the next line of output. Caller is
			// responsible for deleting the string. Returns NULL if output
			// buffer is empty.
			const RString * Next() {
				return shiftLines();
			}

			// Dequeue and copy the next line of output to the given string,
			// then return true. Returns false if output buffer is empty.
			// Caller is not made responsible for any new string object.
			bool Next(RString& line) {
				const RString * item = shiftLines();
				if(item == NULL) {
					return false;
				}
				else {
					line = *item;
					delete item;
					return true;
				}
			}
	};
}

namespace
{
	// Abstract interface for object that retrieves lines from a stream.
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
}

namespace
{
	// Retrieves lines from a cstdio (std::FILE) stream.
	class StdCFileLineReader : public LineReader
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
						if(chomp(line)) {
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

#if !defined(WITHOUT_NETWORKING)
namespace
{
	// Retrieves lines from a socket (EzSockets) stream.
	class EzSocketsLineReader : public LineReader
	{
		private:
			volatile bool keepRunning;
			LineBuffer lines;
			EzSockets * sock;
			static const size_t BUFFER_SIZE = 64;
			RChar readBuffer[BUFFER_SIZE];
			RageMutex mutex;

			inline void shutdown(bool flushWaitingLines) {
				// Cause ReadLine to stop on next cycle.
				keepRunning = false;

				mutex.Lock();

				if(sock != NULL) {
					if(IsOpen()) {
						sock->close();
					}
					delete sock;
					sock = NULL;

					// Only skip this from destructor
					if(flushWaitingLines) {
						lines.Flush();
					}
				}

				mutex.Unlock();
			}

			// Checked when the ReadLine loop encounters a zero-length read
			// or no data is available
			inline bool shouldStopRunningAfterNonRead() {
				// We don't have a meaningful way to recover from a
				// disconnection nor from an "exceptional condition", so a
				// disconnect or an error stops the loop.

				// If this object was closed independently, the loop will
				// stop on the next check anyway. (Close() clears
				// keepRunning outside of the mutex.)

				// If the socket is open and not in an error state, then it
				// simply had no data to return. So, we try again
				// immediately. The timeout parameter to DataAvailable()
				// prevents this from being full-fledged busy waiting.
				return ((sock->state == EzSockets::skDISCONNECTED) || sock->IsError());
			}

			inline bool unlockedReadLine(RString& line) {
				// If the buffer already contains a line
				if(lines.Next(line)) {
					return true;
				}

				// If the buffer contains no full lines
				{
					size_t readLen;

					while(keepRunning && IsOpen()) {
						readLen = sock->DataAvailable(DEFAULT_TIMEOUT_MS) ?
							sock->ReadData((char*)readBuffer, sizeof(readBuffer)) : 0;

						if(readLen > 0) {
							if(lines.AddData(readBuffer, readLen)) {
								lines.Next(line);
								return true;
							}
						}
						else {
							// No data was read.
							if(shouldStopRunningAfterNonRead()) {
								keepRunning = false;
							}
						}
					}
				}

				// At this point, the object should no longer receive input.
				// If the loop stopped due to keepRunning being set to false
				// (including when the socket has disconnected or errored),
				// the socket may still be open.
				//
				// Close() clears keepRunning, closes the socket, and
				// flushes remaining input to lines. (Also, it can be
				// repeated safely.) The line buffer remains operational, so
				// the remaining lines can still be retrieved from
				// subsequent ReadLine()s.
				Close();

				// There might be a line in the buffer after the flush.
				return lines.Next(line);
			}

		public:
			EzSocketsLineReader(EzSockets * sock) :
				keepRunning(true),
				mutex(mutexName("EzSocketsLineReader", this))
			{
				this->sock = sock;
			}

			~EzSocketsLineReader() {
				shutdown(false);
			}

			virtual bool IsOpen() {
				return ((sock != NULL) && (sock->state != EzSockets::skDISCONNECTED));
			}

			virtual bool ReadLine(RString& line) {
				bool result = false;
				mutex.Lock();
				result = unlockedReadLine(line);
				mutex.Unlock();
				return result;
			}

			virtual void Close() {
				shutdown(true);
			}
	};
}
#endif // !defined(WITHOUT_NETWORKING)

namespace
{
	class TakeOneLineReader
	{
		private:
			LineReader * item;
			RageMutex mutex;

		public:
			TakeOneLineReader(LineReader * reader) :
				item(reader),
				mutex(mutexName("TakeOneLineReader", this))
			{
				this->item = item;
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

namespace X_InputHandler_SextetStream
{
	static Impl * CreateImpl(InputHandler_SextetStream * _this, LineReader * reader);

	class Impl
	{
		private:
			InputHandler_SextetStream * handler;
			TakeOneLineReader takeReader;

		protected:
			uint8_t stateBuffer[STATE_BUFFER_SIZE];
			RageThread inputThread;
			bool continueInputThread;

			void ButtonPressed(const DeviceInput& di)
			{
				handler->ButtonPressed(di);
			}

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

			Impl(InputHandler_SextetStream * _this, LineReader * reader) :
				handler(_this), takeReader(reader)
			{
				LOG->Info("Number of button states supported by current InputHandler_SextetStream: %u",
					(unsigned)BUTTON_COUNT);
				continueInputThread = false;

				clearStateBuffer();
				createThread();
			}

		public:

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
					if(isValidSextet(b)) {
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

			friend Impl * CreateImpl(InputHandler_SextetStream * _this, LineReader * reader);
	};

	static Impl * CreateImpl(InputHandler_SextetStream * _this, LineReader * reader)
	{
		return new Impl(_this, reader);
	}
}


void InputHandler_SextetStream::GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut)
{
	if(_impl != NULL) {
		_impl->GetDevicesAndDescriptions(vDevicesOut);
	}
}

InputHandler_SextetStream::InputHandler_SextetStream()
{
	_impl = NULL;
}

InputHandler_SextetStream::~InputHandler_SextetStream()
{
	if(_impl != NULL) {
		delete _impl;
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
	_impl = CreateImpl(this, new StdCFileLineReader(file));
}

InputHandler_SextetStreamFromFile::InputHandler_SextetStreamFromFile(const RString& filename)
{
	_impl = CreateImpl(this, StdCFileLineReader::Open(filename));
}

InputHandler_SextetStreamFromFile::InputHandler_SextetStreamFromFile()
{
	_impl = CreateImpl(this, StdCFileLineReader::Open(g_sSextetStreamInputFilename));
}


#if !defined(WITHOUT_NETWORKING)
// SextetStreamFromSocket

#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT 5733

static Preference<RString> g_sSextetStreamInputSocketHost("SextetStreamInputSocketHost", DEFAULT_HOST);
static Preference<int> g_sSextetStreamInputSocketPort("SextetStreamInputSocketPort", DEFAULT_PORT);

inline LineReader * openInputSocket(const RString& host, unsigned short port)
{
	EzSockets * sock = new EzSockets;

	sock->close();
	sock->create();
	sock->blocking = false;

	if(!sock->connect(host, port)) {
		LOG->Warn("Could not connect to host '%s' port %u for input", host.c_str(), (unsigned int)port);
		delete sock;
		sock = NULL;
	}

	return new EzSocketsLineReader(sock);
}

InputHandler_SextetStreamFromSocket::InputHandler_SextetStreamFromSocket(EzSockets * sock)
{
	_impl = CreateImpl(this, new EzSocketsLineReader(sock));
}

InputHandler_SextetStreamFromSocket::InputHandler_SextetStreamFromSocket(const RString& host, unsigned short port)
{
	_impl = CreateImpl(this, openInputSocket(host, port));
}

InputHandler_SextetStreamFromSocket::InputHandler_SextetStreamFromSocket()
{
	RString host = g_sSextetStreamInputSocketHost;
	int port = g_sSextetStreamInputSocketPort;
	_impl = CreateImpl(this, openInputSocket(host, (unsigned short)port));
}

#endif // !defined(WITHOUT_NETWORKING)




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


