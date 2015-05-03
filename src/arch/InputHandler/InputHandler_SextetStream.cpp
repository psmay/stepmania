#include "global.h"
#include "InputHandler_SextetStream.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "RageThreads.h"
#include "RageUtil.h"

#include "arch/Sextets/IO/StdCFileLineReader.h"
#include "arch/Sextets/Threads/SingleReadPointerVariable.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

using namespace std;
using namespace Sextets::IO;
using namespace Sextets::Threads;

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


// The guts of an InputHandler_SextetStream based on a LineReader.
class InputHandler_SextetStream::_Impl
{
	private:
		InputHandler_SextetStream * handler;
		SingleReadPointerVariable<LineReader> readerVar;

		uint8_t currentState[STATE_BUFFER_SIZE];
		RageThread inputThread;
		bool continueInputLoop;

		inline void updateButtonState(const InputDevice& device,
			size_t buttonIndex, bool value, const RageTimer& now)
		{
			DeviceInput di(device, deviceButtonForIndex(buttonIndex), value, now);
			handler->ButtonPressed(di);
		}

		inline void clearCurrentState()
		{
			memset(currentState, 0, STATE_BUFFER_SIZE);
		}

		inline void createThread()
		{
			continueInputLoop = true;
			inputThread.SetName("SextetStream input thread");
			inputThread.Create(runInputLoopThread, this);
		}

		void runInputLoop()
		{
			RString line;
			Sextets::IO::LineReader * reader;

			LOG->Trace("Input thread started; getting line reader");
			reader = readerVar.Claim();

			if(reader == NULL) {
				LOG->Warn("Line reader for SextetStream input missing or already used");
			}
			else {
				LOG->Trace("Got line reader");
				while(continueInputLoop) {
					LOG->Trace("Reading line");
					if(reader->ReadLine(line)) {
						LOG->Trace("Got line: '%s'", line.c_str());
						if(line.length() > 0) {
							uint8_t newState[STATE_BUFFER_SIZE];
							getNewState(newState, line);
							updateButtonsWithStateChanges(newState);
						}
					}
					else {
						if(!reader->IsValid()) {
							// Error or EOF condition.
							LOG->Info("Reached end of SextetStream input");
							continueInputLoop = false;
						}
						// Otherwise, the line simply may not have finished
						// buffering before the timeout. Keep going.
					}
				}
				LOG->Info("SextetStream input stopped");
				delete reader;
			}
		}

		static int runInputLoopThread(void * p)
		{
			((_Impl*) p)->runInputLoop();
			return 0;
		}

		inline void getNewState(uint8_t * buffer, RString& line)
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

		static inline DeviceButton deviceButtonForIndex(size_t index)
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

		inline void updateButtonsWithStateChanges(const uint8_t * newState)
		{
			InputDevice device = InputDevice(FIRST_DEVICE);
			uint8_t changes[STATE_BUFFER_SIZE];
			RageTimer now;

			// XOR to find differences
			for(size_t i = 0; i < STATE_BUFFER_SIZE; ++i) {
				changes[i] = currentState[i] ^ newState[i];
			}

			// Report on changes
			for(size_t m = 0; m < STATE_BUFFER_SIZE; ++m) {
				for(size_t n = 0; n < 6; ++n) {
					size_t buttonIndex = (m * 6) + n;
					if(buttonIndex < BUTTON_COUNT) {
						if(changes[m] & (1 << n)) {
							bool value = newState[m] & (1 << n);
							LOG->Trace("SS button index %zu %s", buttonIndex, value ? "pressed" : "released");
							updateButtonState(device, buttonIndex, value, now);
						}
					}
				}
			}

			// Update current state
			memcpy(currentState, newState, STATE_BUFFER_SIZE);
		}

	public:
		_Impl(InputHandler_SextetStream * handler, LineReader * reader) :
			handler(handler),
			readerVar(reader),
			continueInputLoop(false)
		{
			LOG->Trace("Number of button states supported by current InputHandler_SextetStream: %u",
				(unsigned)BUTTON_COUNT);

			clearCurrentState();
			createThread();
		}

		~_Impl()
		{
			if(inputThread.IsCreated()) {
				continueInputLoop = false;
				inputThread.Wait();
			}
		}

		void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut)
		{
			vDevicesOut.push_back(InputDeviceInfo(FIRST_DEVICE, "SextetStream"));
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


InputHandler_SextetStreamFromFile::InputHandler_SextetStreamFromFile(FILE * file)
{
	_impl = new InputHandler_SextetStreamFromFile::_Impl(this,
		StdCFileLineReader::Create(file));
}

InputHandler_SextetStreamFromFile::InputHandler_SextetStreamFromFile(const RString& filename)
{
	_impl = new InputHandler_SextetStreamFromFile::_Impl(this,
		StdCFileLineReader::Create(filename));
}

InputHandler_SextetStreamFromFile::InputHandler_SextetStreamFromFile()
{
	_impl = new InputHandler_SextetStreamFromFile::_Impl(this,
		StdCFileLineReader::Create(g_sSextetStreamInputFilename));
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


