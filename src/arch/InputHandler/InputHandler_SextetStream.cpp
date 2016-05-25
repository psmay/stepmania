#include "global.h"
#include "InputHandler_SextetStream.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "RageThreads.h"
#include "RageUtil.h"

#include "SextetStream/IO/PacketReader.h"
#include "SextetStream/IO/StdCFilePacketReader.h"
#include "SextetStream/Data.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

using namespace std;
using namespace SextetStream::IO;
using namespace SextetStream::Data;

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
	// Gets the DeviceButton that corresponds with the given index.
	//
	// The first COUNT_JOY_BUTTON indices are mapped to the range starting
	// with FIRST_JOY_BUTTON.
	//
	// The next COUNT_KEY indices are mapped to the range starting with
	// FIRST_KEY.
	//
	// Any indices after these are mapped to DeviceButton_Invalid.
	inline DeviceButton ButtonAtIndex(size_t index)
	{
		if(index < COUNT_JOY_BUTTON) {
			return enum_add2(FIRST_JOY_BUTTON, index);
		} else if(index < COUNT_JOY_BUTTON + COUNT_KEY) {
			return enum_add2(FIRST_KEY, index - COUNT_JOY_BUTTON);
		} else {
			return DeviceButton_Invalid;
		}
	}
}

class InputHandler_SextetStream::Impl
{
private:
	uint8_t stateBuffer[STATE_BUFFER_SIZE];
	InputHandler_SextetStream * handler;
	PacketReaderEventGenerator * eventGenerator;
	InputDevice id;
	RageTimer now;

	static void TriggerUpdateButton(void * p, size_t index, bool value)
	{
		((Impl*)p)->UpdateButton(index, value);
	}

	static void TriggerOnReadPacket(void * p, const RString& packet)
	{
		((Impl*)p)->OnReadPacket(packet);
	}

	void UpdateButton(size_t index, bool value)
	{
		DeviceInput di = DeviceInput(id, ButtonAtIndex(index), value, now);
		handler->ButtonPressed(di);
	}

	void OnReadPacket(const RString& packet)
	{
		uint8_t newStateBuffer[STATE_BUFFER_SIZE];
		uint8_t changes[STATE_BUFFER_SIZE];

		ConvertPacketToState(newStateBuffer, STATE_BUFFER_SIZE, packet);
		XorBuffers(changes, stateBuffer, newStateBuffer, STATE_BUFFER_SIZE);

		// Update state
		memcpy(stateBuffer, newStateBuffer, STATE_BUFFER_SIZE);

		// Update device input states
		id = InputDevice(FIRST_DEVICE);
		now = RageTimer();

		// Trigger button presses
		ProcessChanges(newStateBuffer, changes, STATE_BUFFER_SIZE, BUTTON_COUNT, this, TriggerUpdateButton);
	}


public:
	Impl(InputHandler_SextetStream * handler, PacketReader * packetReader)
	{
		LOG->Info("Number of button states supported by current InputHandler_SextetStream: %u",
				  (unsigned)BUTTON_COUNT);

		this->handler = handler;
		
		// Clear the state buffer initially.
		memset(stateBuffer, 0, STATE_BUFFER_SIZE);

		eventGenerator = PacketReaderEventGenerator::Create(packetReader, (void*) this, TriggerOnReadPacket);

		if(eventGenerator == NULL) {
			LOG->Warn("Failed to get PacketReader event generator; this input handler is disabled.");
		}
	}

	virtual ~Impl()
	{
		if(eventGenerator != NULL) {
			delete eventGenerator;
			eventGenerator = NULL;
		}
	}

};

// ctor and dtor of InputHandler_SextetStream.
// If _impl is non-NULL at dtor time, it will be deleted.
//
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

void InputHandler_SextetStream::GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut)
{
	vDevicesOut.push_back(InputDeviceInfo(FIRST_DEVICE, "SextetStream"));
}

// SextetStreamFromFile

REGISTER_INPUT_HANDLER_CLASS(SextetStreamFromFile);

#if defined(_WINDOWS)
	#define DEFAULT_INPUT_FILENAME "\\\\.\\pipe\\StepMania-Input-SextetStream"
#else
	#define DEFAULT_INPUT_FILENAME "Data/StepMania-Input-SextetStream.in"
#endif
static Preference<RString> g_sSextetStreamInputFilename("SextetStreamInputFilename", DEFAULT_INPUT_FILENAME);

InputHandler_SextetStreamFromFile::InputHandler_SextetStreamFromFile()
{
	_impl = new InputHandler_SextetStream::Impl(this, StdCFilePacketReader::Create(g_sSextetStreamInputFilename));
}

InputHandler_SextetStreamFromFile::~InputHandler_SextetStreamFromFile()
{
}


/*
 * Copyright © 2014-2016 Peter S. May
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


