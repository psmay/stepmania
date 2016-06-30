
#include "Sextets/IO/PosixSelectFifoPacketWriter.h"

#if defined(SEXTETS_HAS_POSIX)

#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

#include "RageThreads.h"
#include "RageLog.h"

namespace
{
	int OpenFd(const RString& filename)
	{
		int fd = open(filename.c_str(), O_WRONLY | O_NONBLOCK);

		if(fd < 0) {
			return -1;
		}

		return fd;
	}

	static const size_t timeout = 1000;
}

namespace
{
	using namespace Sextets;
	using namespace Sextets::IO;

	class Impl : public PosixSelectFifoPacketWriter
	{
	private:
		int fd;
		bool seenError;
		RageMutex WritingLock;

		volatile bool stopRequested;

		bool shouldContinue()
		{
			return !stopRequested && IsReady() && !seenError;
		}

		bool writePacket0(const Packet& packet)
		{
			if(stopRequested) {
				return false;
			}

			RString line = packet.GetLine() + "\n";
			size_t remaining = line.size();
			const RString::value_type * data = line.data();

			while(shouldContinue() && (remaining > 0)) {
				struct timeval timeoutval = { 0, timeout * 1000 };
				fd_set set;
				FD_ZERO(&set);
				FD_SET(fd, &set);

				int event_count = select(fd + 1, NULL, &set, NULL, &timeoutval);

				if(event_count == 0) {
					// Timed out; check condition then keep waiting
					continue;
				} else if(event_count < 0) {
					LOG->Warn("Problem waiting for write on stream: %s", std::strerror(errno));
					seenError = true;
					return false;
				} else if(FD_ISSET(fd, &set)) {
					// There may now be room on our stream
					ssize_t count = write(fd, data, remaining);
					if(count >= 0) {
						// The write was successful (though it may have been empty).
						data += count;
						remaining -= count;
					} else {
						if(errno == EINTR) {
							// Caught an interrupt, so recheck the condition
							continue;
						} else if((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
							// Could not write this time, but try again
							continue;
						} else {
							LOG->Warn("Problem writing to stream: %s", std::strerror(errno));
							seenError = true;
							return false;
						}
					}
				}
			}
			return true;
		}

	public:
		Impl(const RString& filename) : WritingLock("WritingLock")
		{
			stopRequested = false;

			LOG->Info(
				"Sextets POSIX select() FIFO packet writer opening stream from '%s' for output", filename.c_str());

			fd = OpenFd(filename);
			seenError = false;

			if(!HasStream()) {
				LOG->Warn(
					"Sextets POSIX select() FIFO packet writer could not open stream from '%s' for output", filename.c_str());
				return;
			}
		}

		~Impl()
		{
			Close();
		}

		bool HasStream()
		{
			return fd >= 0;
		}

		bool IsReady()
		{
			return HasStream() && !stopRequested && !seenError;
		}

		bool WritePacket(const Packet& packet)
		{
			WritingLock.Lock();
			bool result = writePacket0(packet);
			WritingLock.Unlock();

			return result;
		}

		void Close()
		{
			// If writing loop is underway, have it quit on the next pass
			stopRequested = true;

			// Wait until any writing loop has stopped
			WritingLock.Lock();

			// Close fd and mark closed
			if(fd >= 0) {
				close(fd);
				fd = -1;
			}

			// Releaase lock
			WritingLock.Unlock();
		}

	};
}

namespace Sextets
{
	namespace IO
	{
		PosixSelectFifoPacketWriter* PosixSelectFifoPacketWriter::Create(const RString& filename)
		{
			Impl * impl = new Impl(filename);
			return impl->HasStream() ? impl : NULL;
		}

		PosixSelectFifoPacketWriter::~PosixSelectFifoPacketWriter() {}
	}
}

#endif // defined(SEXTETS_HAS_POSIX)

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
