#ifndef InputHandler_SextetStream_h
#define InputHandler_SextetStream_h

#include "InputHandler.h"

#include "Sextets/IO/PlatformFifo.h"

#include <cstdio>

#include "Sextets/IO/PacketReaderEventGenerator.h"

class InputHandler_SextetStream: public InputHandler
{
public:
	InputHandler_SextetStream();
	virtual ~InputHandler_SextetStream();
	virtual void Update();
	virtual void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut);

public:
	class Impl;
	friend class Impl;
protected:
	Impl * _impl;
};

class InputHandler_SextetStreamFromFile: public InputHandler_SextetStream
{
public:
	virtual ~InputHandler_SextetStreamFromFile();
	InputHandler_SextetStreamFromFile();
};

#if defined(SEXTETS_FIFO_READER)
class InputHandler_SextetStreamFromFifo: public InputHandler_SextetStream
{
public:
	virtual ~InputHandler_SextetStreamFromFifo();
	InputHandler_SextetStreamFromFifo();
};
#endif // defined(SEXTETS_FIFO_READER)

#endif

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
