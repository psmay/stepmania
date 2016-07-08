
#ifndef Sextets_IO_Windows__BasicOverlappedNamedPipe_h
#define Sextets_IO_Windows__BasicOverlappedNamedPipe_h

#include "global.h"

namespace Sextets
{
	namespace IO
	{
		namespace Windows
		{
			// Simplified access to Windows named pipes, using OVERLAPPED to
			// implement blocking timeout/condition check loops.
			//
			// In general, this class is not synchronized, so external
			// synchronization is required for use among multiple threads.
			//
			// The sole exception is RequestClose(), which can be called at any
			// time from any thread, that sets a flag that will be seen at the
			// next condition check and cue the object to close.
			//
			// The condition check automatically closes this object, cancelling
			// any pending operations if necessary, if IsReady() would return
			// false.
			//
			// The flag is checked at the beginning of a read or write as well
			// as within the condition check loop of either. If no such
			// operation is underway, the object will remain nominally open
			// until the next check.
			//
			// IsReady() returns true unless any of the following conditions are
			// true:
			// - an error has occurred
			// - an EOF has been encountered
			// - the pipe was never opened
			// - the pipe has already been closed
			//
			// HasHandle() returns true unless the pipe was never opened or has
			// already been closed.
			class _BasicOverlappedNamedPipe
			{
				public:
					virtual ~_BasicOverlappedNamedPipe() {}
					virtual bool HasHandle() = 0;
					virtual bool Read(void * buffer, size_t bufferLength, size_t& receivedLength) = 0;
					virtual bool Write(const void * data, size_t dataLength, size_t& sentLengthOut) = 0;
					virtual void RequestClose() = 0;
					virtual void Close() = 0;
					virtual bool IsReady() = 0;

					// Returns NULL if setting up the pipe failed. This does not
					// wait for a busy pipe to free up since this application is
					// only used for 1:1 connections.
					static _BasicOverlappedNamedPipe * Create(const RString& pipePath, bool forRead, bool forWrite);
			};
		}
	}
}


#endif // Sextets_IO_Windows__BasicOverlappedNamedPipe_h

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
