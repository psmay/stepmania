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

#define _DEVICE DEVICE_JOY1
#define DEFAULT_TIMEOUT_MS 1000
#define BUTTON_COUNT 64
#define STATE_BUFFER_SIZE NUMBER_OF_SEXTETS_FOR_BIT_COUNT(BUTTON_COUNT)

#define _Base InputHandler_SextetStream
#define _BaseImpl _Base::Impl
#define _FromFile InputHandler_SextetStreamFromFile

namespace
{
	class LineReader
	{
		protected:
			int timeout_ms;

		public:
			LineReader()
			{
				timeout_ms = DEFAULT_TIMEOUT_MS;
			}

			virtual ~LineReader()
			{
			}

			virtual bool IsValid()
			{
				return false;
			}

			// Ideally, this method should return if timeout_ms passes
			// before a line becomes available. Actually doing this may
			// require some platform-specific non-blocking read capability.
			// This sort of thing could be implemented using e.g. POSIX
			// select() and Windows GetOverlappedResultEx(), both of which
			// have timeout parameters. (I get the sense that the
			// RageFileDriverTimeout class could be convinced to work, but
			// there isn't a lot of code using it, so I'm lacking the proper
			// examples.)
			//
			// If this method does block, almost everything will still work,
			// but the blocking may prevent the loop from checking
			// continueInputThread in a timely fashion. If the stream ceases
			// to produce new data before this object is destroyed, the
			// current thread will hang until the other side of the
			// connection closes the stream (or produces a line of data). A
			// workaround for that would be to have the far side of the
			// connection repeat its last line every second or so as a
			// keepalive.
			//
			// false (line undefined) if there is an error or EOF condition,
			// true (line = next line from stream) if a whole line is available,
			// true (line = "") if no error but still waiting for next line.
			virtual bool ReadLine(RString& line) = 0;
	};
}

class _Base::Impl
{
	private:
		_Base * handler;

	protected:
		void ButtonPressed(const DeviceInput& di)
		{
			handler->ButtonPressed(di);
		}

	protected:
		uint8_t stateBuffer[STATE_BUFFER_SIZE];
		size_t timeout_ms;
		RageThread inputThread;
		bool continueInputThread;

		// Construct and return the LineReader that makes sense for this
		// object. getLineReader() calls this; if the returned object claims
		// it is valid, it is returned. Otherwise, it is destroyed and NULL
		// is returned.
		virtual LineReader * getUnvalidatedLineReader() = 0;

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

		LineReader * getLineReader()
		{
			LineReader * linereader = getUnvalidatedLineReader();
			if(linereader != NULL) {
				if(!linereader->IsValid()) {
					delete linereader;
					linereader = NULL;
				}
			}
			return linereader;
		}

	public:
		Impl(_Base * _this)
		{
			continueInputThread = false;
			timeout_ms = DEFAULT_TIMEOUT_MS;

			handler = _this;
			clearStateBuffer();
			createThread();
		}

		virtual ~Impl()
		{
			if(inputThread.IsCreated()) {
				continueInputThread = false;
				inputThread.Wait();
			}
		}

		virtual void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut)
		{
			vDevicesOut.push_back(InputDeviceInfo(_DEVICE, "SextetStream"));
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

			// Copy from line to buffer until either it is full or we've run out
			// of characters. Characters outside the sextet code range
			// (0x30..0x6F) are skipped; the remaining characters have their two
			// high bits cleared.
			for(i = 0; i < lineLen; ++i) {
				char b = line[i];
				if((b >= 0x30) && (b <= 0x6F)) {
					buffer[cursor] = b & 0x3F;
					++cursor;
					if(cursor >= STATE_BUFFER_SIZE) {
						break;
					}
				}
			}
		}

		inline DeviceButton JoyButtonAtIndex(size_t index)
		{
			return enum_add2(JOY_BUTTON_1, index);
		}

		inline void ReactToChanges(const uint8_t * newStateBuffer)
		{
			InputDevice id = InputDevice(_DEVICE);
			uint8_t changes[STATE_BUFFER_SIZE];
			RageTimer now;

			// XOR to find differences
			for(size_t i = 0; i < STATE_BUFFER_SIZE; ++i) {
				changes[i] = stateBuffer[i] ^ newStateBuffer[i];
			}

			// Report on changes
			for(size_t m = 0; m < STATE_BUFFER_SIZE; ++m) {
				for(size_t n = 0; n < 6; ++n) {
					size_t bi = (m * 6) + n;
					if(bi < BUTTON_COUNT) {
						if(changes[m] & (1 << n)) {
							bool value = newStateBuffer[m] & (1 << n);
							LOG->Trace("SS button index %zu %s", bi, value ? "pressed" : "released");
							DeviceInput di = DeviceInput(id, JoyButtonAtIndex(bi), value, now);
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
			LineReader * linereader;

			LOG->Trace("Input thread started; getting line reader");
			linereader = getLineReader();

			if(linereader == NULL) {
				LOG->Warn("Could not open line reader for SextetStream input");
			}
			else {
				LOG->Trace("Got line reader");
				while(continueInputThread) {
					LOG->Trace("Reading line");
					if(linereader->ReadLine(line)) {
						LOG->Trace("Got line: '%s'", line.c_str());
						if(line.length() > 0) {
							uint8_t newStateBuffer[STATE_BUFFER_SIZE];
							GetNewState(newStateBuffer, line);
							ReactToChanges(newStateBuffer);
						}
					}
					else {
						// Error or EOF condition.
						LOG->Trace("Reached end of SextetStream input");
						continueInputThread = false;
					}
				}
				LOG->Info("SextetStream input stopped");
				delete linereader;
			}
		}
};

void _Base::GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut)
{
	if(_impl != NULL) {
		_impl->GetDevicesAndDescriptions(vDevicesOut);
	}
}

_Base::_Base()
{
	_impl = NULL;
}

_Base::~_Base()
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

namespace
{
	class RageFileLineReader: public LineReader
	{
		protected:
			RageFile * file;

		public:
			RageFileLineReader(RageFile * rageFile)
			{
				LOG->Info("Starting InputHandler_SextetStreamFromFile from open RageFile");
				file = rageFile;
			}

			RageFileLineReader(const RString& filename)
			{
				LOG->Info("Starting InputHandler_SextetStreamFromFile from RageFile with filename '%s'",
					filename.c_str());
				file = new RageFile;

				if(!file->Open(filename, RageFile::READ)) {
					LOG->Warn("Error opening file '%s' for input (RageFile): %s", filename.c_str(),
						file->GetError().c_str());
					SAFE_DELETE(file);
					file = NULL;
				}
				else {
					LOG->Trace("File opened");
				}
			}

			~RageFileLineReader()
			{
				if(file != NULL) {
					file->Close();
					SAFE_DELETE(file);
				}
			}

			virtual bool IsValid()
			{
				return file != NULL;
			}

			virtual bool ReadLine(RString& line)
			{
				return (file == NULL) ? false : (file->GetLine(line) > 0);
			}
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
			StdCFileLineReader(std::FILE * file)
			{
				LOG->Info("Starting InputHandler_SextetStreamFromFile from open std::FILE");
				this->file = file;
			}

			StdCFileLineReader(const RString& filename)
			{
				LOG->Info("Starting InputHandler_SextetStreamFromFile from std::FILE with filename '%s'",
					filename.c_str());
				file = std::fopen(filename.c_str(), "rb");

				if(file == NULL) {
					LOG->Warn("Error opening file '%s' for input (cstdio): %s", filename.c_str(),
						std::strerror(errno));
				}
				else {
					LOG->Trace("File opened");
					// Disable buffering on the file
					std::setbuf(file, NULL);
				}
			}

			~StdCFileLineReader()
			{
				if(file != NULL) {
					std::fclose(file);
				}
			}

			virtual bool IsValid()
			{
				return file != NULL;
			}

			virtual bool ReadLine(RString& line)
			{
				bool afterFirst = false;
				size_t len;

				line = "";

				if(file != NULL) {
					while(fgets(buffer, BUFFER_SIZE, file) != NULL) {
						afterFirst = true;
						line += buffer;
						len = line.length();
						if(len > 0 && line[len - 1] == 0xA) {
							break;
						}
					}
				}

				return afterFirst;
			}
	};

	class RageFileHandleImpl: public _BaseImpl
	{
		protected:
			RageFile * file;

		public:
			RageFileHandleImpl(_FromFile * handler, RageFile * file) :
				_BaseImpl(handler)
			{
				this->file = file;
			}

			virtual LineReader * getUnvalidatedLineReader()
			{
				return new RageFileLineReader(this->file);
			}

			virtual ~RageFileHandleImpl()
			{
				// line reader dtor will destroy file for us
			}
	};

	class RageFileNameImpl: public _BaseImpl
	{
		protected:
			RString filename;

		public:
			RageFileNameImpl(_FromFile * handler, const RString& filename) :
				_BaseImpl(handler)
			{
				this->filename = filename;
			}

			virtual LineReader * getUnvalidatedLineReader()
			{
				return new RageFileLineReader(filename);
			}

			virtual ~RageFileNameImpl()
			{
				// Nothing to destroy
			}
	};

	class StdCFileHandleImpl: public _BaseImpl
	{
		protected:
			std::FILE * file;

		public:
			StdCFileHandleImpl(_FromFile * handler, std::FILE * file) :
				_BaseImpl(handler)
			{
				this->file = file;
			}

			virtual LineReader * getUnvalidatedLineReader()
			{
				return new StdCFileLineReader(this->file);
			}

			virtual ~StdCFileHandleImpl()
			{
				// line reader dtor will close file for us
			}
	};

	class StdCFileNameImpl: public _BaseImpl
	{
		protected:
			RString filename;

		public:
			StdCFileNameImpl(_FromFile * handler, const RString& filename) :
				_BaseImpl(handler)
			{
				this->filename = filename;
			}

			virtual LineReader * getUnvalidatedLineReader()
			{
				return new StdCFileLineReader(filename);
			}

			virtual ~StdCFileNameImpl()
			{
				// Nothing to destroy
			}
	};
}

#ifdef INPUT_HANDLER_SEXTET_STREAM_USE_RAGEFILE_DEFAULT
#define DefaultFileNameImpl RageFileNameImpl
#else
#define DefaultFileNameImpl StdCFileNameImpl
#endif

_FromFile::_FromFile(RageFile * file)
{
	_impl = new RageFileHandleImpl(this, file);
}

_FromFile::_FromFile(FILE * file)
{
	_impl = new StdCFileHandleImpl(this, file);
}

_FromFile::_FromFile(const RString& filename)
{
	_impl = new DefaultFileNameImpl(this, filename);
}

_FromFile::_FromFile()
{
	_impl = new DefaultFileNameImpl(this, g_sSextetStreamInputFilename);
}

/*
 * Copyright © 2014 Peter S. May
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


