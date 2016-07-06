
#ifndef Sextets_IO_PlatformFifo_h
#define Sextets_IO_PlatformFifo_h

#include "Sextets/Platform.h"

#if defined(SEXTETS_HAVE_WINDOWS)
	#include "Sextets/IO/Windows/NamedFifoPacketReader.h"
	#include "Sextets/IO/Windows/NamedFifoPacketWriter.h"
	#define SEXTETS_FIFO_READER Sextets::IO::Windows::NamedFifoPacketReader
	#define SEXTETS_FIFO_WRITER Sextets::IO::Windows::NamedFifoPacketWriter
#elif defined(SEXTETS_HAVE_POSIX)
	#include "Sextets/IO/Posix/NamedFifoPacketReader.h"
	#include "Sextets/IO/Posix/NamedFifoPacketWriter.h"
	#define SEXTETS_FIFO_READER Sextets::IO::Posix::NamedFifoPacketReader
	#define SEXTETS_FIFO_WRITER Sextets::IO::Posix::NamedFifoPacketWriter
#else
	#undef SEXTETS_FIFO_READER
	#undef SEXTETS_FIFO_WRITER
#endif

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
