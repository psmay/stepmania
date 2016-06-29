#ifndef Sextets_IO_WindowsOverlappedPipePacketReader_h
#define Sextets_IO_WindowsOverlappedPipePacketReader_h

// THIS IS ONLY A PLACEHOLDER
// Don't expect this code to work (or even allow the entire program to work) in its current form.

#include "Sextets/Platform.h"

#if defined(SEXTETS_HAS_WINDOWS)

#include "Sextets/IO/PacketReader.h"

namespace Sextets
{
	namespace IO
	{
		// PacketReader implementation using POSIX read() and select()
		class WindowsOverlappedPipePacketReader : public PacketReader
		{
			public:
				virtual ~WindowsOverlappedPipePacketReader();
				static WindowsOverlappedPipePacketReader* Create(const RString& filename);
		};
	}
}

#endif // defined(SEXTETS_HAS_WINDOWS)

#endif

/*
 * Copyright © 2016 Peter S. May
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
