#ifndef Sextets_IO_PosixSelectFifoPacketWriter_h
#define Sextets_IO_PosixSelectFifoPacketWriter_h

#include "Sextets/Platform.h"

#if defined(SEXTETS_HAS_POSIX)

#include "Sextets/IO/PacketWriter.h"

namespace Sextets
{
	namespace IO
	{
		class PosixSelectFifoPacketWriter : public PacketWriter
		{
		public:
			virtual ~PosixSelectFifoPacketWriter();

			// Note: If there is a problem opening the file, returns NULL.
			static PosixSelectFifoPacketWriter * Create(const RString& filename);
		};
	}
}

#endif // defined(SEXTETS_HAS_POSIX)

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
