#include "global.h"
#include "InputHandler_SextetStream.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "RageThreads.h"
#include "RageUtil.h"

#include "arch/Sextets/IO/StdCFileLineReader.h"

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

#define STATE_BUFFER_SIZE NUMBER_OF_SEXTETS_FOR_BIT_COUNT(BUTTON_COUNT)



class InputHandler_SextetStream::Impl
{
	private:
		InputHandler_SextetStream * handler;

	protected:
		void ButtonPressed(const DeviceInput& di)
		{
			handler->ButtonPressed(di);
		}

		uint8_t stateBuffer[STATE_BUFFER_SIZE];
		RageThread inputThread;
		bool continueInputThread;

		// Construct and return the LineReader that makes sense for this
		// object. getLineReader() calls this; if the returned object claims
		// it is valid, it is returned. Otherwise, it is destroyed and NULL
		// is returned.
		virtual Sextets::IO::LineReader * getUnvalidatedLineReader() = 0;

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

		Sextets::IO::LineReader * getLineReader()
		{
			Sextets::IO::LineReader * linereader = getUnvalidatedLineReader();
			if(linereader != NULL) {
				if(!linereader->IsValid()) {
					delete linereader;
					linereader = NULL;
				}
			}
			return linereader;
		}

	public:
		Impl(InputHandler_SextetStream * _this)
		{
			LOG->Info("Number of button states supported by current InputHandler_SextetStream: %u",
				(unsigned)BUTTON_COUNT);
			continueInputThread = false;

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
			for(size_t m = 0; m < STATE_BUFFER_SIZE; ++m) {
				for(size_t n = 0; n < 6; ++n) {
					size_t bi = (m * 6) + n;
					if(bi < BUTTON_COUNT) {
						if(changes[m] & (1 << n)) {
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
			Sextets::IO::LineReader * linereader;

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

namespace
{
	class StdCFileHandleImpl: public InputHandler_SextetStream::Impl
	{
		protected:
			std::FILE * file;

		public:
			StdCFileHandleImpl(InputHandler_SextetStreamFromFile * handler, std::FILE * file) :
				InputHandler_SextetStream::Impl(handler)
			{
				this->file = file;
			}

			virtual Sextets::IO::LineReader * getUnvalidatedLineReader()
			{
				return Sextets::IO::StdCFileLineReader::Create(this->file);
			}

			virtual ~StdCFileHandleImpl()
			{
				// line reader dtor will close file for us
			}
	};

	class StdCFileNameImpl: public InputHandler_SextetStream::Impl
	{
		protected:
			RString filename;

		public:
			StdCFileNameImpl(InputHandler_SextetStreamFromFile * handler, const RString& filename) :
				InputHandler_SextetStream::Impl(handler)
			{
				this->filename = filename;
			}

			virtual Sextets::IO::LineReader * getUnvalidatedLineReader()
			{
				return Sextets::IO::StdCFileLineReader::Create(filename);
			}

			virtual ~StdCFileNameImpl()
			{
				// Nothing to destroy
			}
	};
}


InputHandler_SextetStreamFromFile::InputHandler_SextetStreamFromFile(FILE * file)
{
	_impl = new StdCFileHandleImpl(this, file);
}

InputHandler_SextetStreamFromFile::InputHandler_SextetStreamFromFile(const RString& filename)
{
	_impl = new StdCFileNameImpl(this, filename);
}

InputHandler_SextetStreamFromFile::InputHandler_SextetStreamFromFile()
{
	_impl = new StdCFileNameImpl(this, g_sSextetStreamInputFilename);
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


