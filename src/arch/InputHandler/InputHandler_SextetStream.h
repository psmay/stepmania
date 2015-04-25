#ifndef INPUT_HANDLER_SEXTETSTREAM
#define INPUT_HANDLER_SEXTETSTREAM

#include "InputHandler.h"
#include <cstdio>

#if !defined(WITHOUT_NETWORKING)
#include "ezsockets.h"
#endif // !defined(WITHOUT_NETWORKING)

namespace X_InputHandler_SextetStream {
	class Impl;
}

class InputHandler_SextetStream: public InputHandler
{
	public:
		InputHandler_SextetStream();
		virtual ~InputHandler_SextetStream();
		//virtual void Update();
		virtual void GetDevicesAndDescriptions(vector<InputDeviceInfo>& vDevicesOut);

	protected:
		X_InputHandler_SextetStream::Impl * _impl;

	private:
		friend class X_InputHandler_SextetStream::Impl;
};

// Note: InputHandler_SextetStreamFromFile uses blocking I/O. For the
// handler thread to close in a timely fashion, the producer of data for the
// file (e.g. the program at the other end of the pipe) must either close
// the file or output and flush a line of data no less often than about once
// per second, even if there has been no change. (Repeating the most recent
// state accomplishes this without triggering any new events.) Either of
// these interrupts the blocking read so that the loop can check its
// continue flag.
class InputHandler_SextetStreamFromFile: public InputHandler_SextetStream
{
	public:
		// Note: In the current implementation, the filename (either the
		// `filename` parameter or the `SextetStreamInputFilename` setting) is
		// passed to fopen(), not a RageFile ctor, so specify the file to be
		// opened on the actual filesystem instead of the mapped filesystem. (I
		// couldn't get RageFile to work here, possibly because I haven't
		// determined how to disable buffering on an input file.) 
		InputHandler_SextetStreamFromFile();
		InputHandler_SextetStreamFromFile(const RString& filename);

		// The file object passed here must already be open and buffering should
		// be disabled. The file object will be closed in the destructor.
		InputHandler_SextetStreamFromFile(std::FILE * file);
};

#if !defined(WITHOUT_NETWORKING)
class InputHandler_SextetStreamFromSocket: public InputHandler_SextetStream
{
	public:
		InputHandler_SextetStreamFromSocket();
		InputHandler_SextetStreamFromSocket(const RString& host, unsigned short port);

		// The socket object passed here must already be open, with blocking set
		// to false. The socket object will be closed and deleted in the
		// destructor.
		InputHandler_SextetStreamFromSocket(EzSockets * sock);
};
#endif // !defined(WITHOUT_NETWORKING)

#endif

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
