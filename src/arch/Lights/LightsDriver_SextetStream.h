#ifndef LightsDriver_SextetStream_h
#define LightsDriver_SextetStream_h

#include "LightsDriver.h"

#include "Sextets/IO/PlatformFifo.h"

class LightsDriver_SextetStream : public LightsDriver
{
	public:
		LightsDriver_SextetStream();
		virtual ~LightsDriver_SextetStream();
		virtual void Set(const LightsState *ls);

	public:
		class Impl;
	protected:
		Impl * _impl;
};

class LightsDriver_SextetStreamToFile : public LightsDriver_SextetStream
{
	public:
		LightsDriver_SextetStreamToFile();
};

#if defined(SEXTETS_FIFO_WRITER)
class LightsDriver_SextetStreamToFifo : public LightsDriver_SextetStream
{
	public:
		LightsDriver_SextetStreamToFifo();
};
#endif // defined(SEXTETS_FIFO_WRITER)


#endif // H

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
