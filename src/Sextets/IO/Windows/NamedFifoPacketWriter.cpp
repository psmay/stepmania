
#include "Sextets/IO/Windows/NamedFifoPacketWriter.h"

#if defined(SEXTETS_HAVE_WINDOWS)

#include <windows.h>

#include "RageThreads.h"
#include "RageLog.h"

namespace
{
	static const size_t TIMEOUT = 1000;
	static const size_t BUSY_PIPE_TIMEOUT = 20000;

	RString GetWindowsErrorMessage(DWORD error)
	{
		LPVOID lpMsgBuf;

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL);

		RString copy = (LPCTSTR) lpMsgBuf;

		LocalFree(lpMsgBuf);

		return copy;
	}

	HANDLE OpenNamedPipe(LPCTSTR pipeName, size_t busyTimeout)
	{
		for(;;) {
			HANDLE ph = CreateFile(pipeName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

			if(ph != INVALID_HANDLE_VALUE) {
				// Success
				return ph;
			}

			DWORD createFileError = GetLastError();

			if(createFileError == ERROR_PIPE_BUSY) {
				// All pipes busy; wait for a free one
				if(WaitNamedPipe(pipeName, (DWORD) busyTimeout)) {
					// Something changed; try again
					continue;
				} else {
					LOG->Warn("Timed out waiting for pipe to become available");
					return INVALID_HANDLE_VALUE;
				}
			} else {
				RString msg = GetWindowsErrorMessage(createFileError);
				LOG->Warn("Could not open pipe: %s", msg.c_str());
				return INVALID_HANDLE_VALUE;
			}
		}
	}

	HANDLE CreateEventForOverlapped()
	{
		HANDLE h = CreateEvent(NULL, TRUE, FALSE, NULL);

		if(h == NULL) {
			RString msg = GetWindowsErrorMessage(GetLastError());
			LOG->Warn("Event for OVERLAPPED structure could not be created: %d", msg.c_str());
		}

		return h;
	}


}

namespace
{
	using namespace Sextets;
	using namespace Sextets::IO;

	class Impl : public Sextets::IO::Windows::NamedFifoPacketWriter
	{
		private:
			HANDLE ph;
			bool seenEof;
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
					struct timeval timeoutval = { 0, TIMEOUT * 1000 };
					fd_set set;
					FD_ZERO(&set);
					FD_SET(ph, &set);

					int event_count = select(ph + 1, NULL, &set, NULL, &timeoutval);

					if(event_count == 0) {
						// Timed out; check condition then keep waiting
						continue;
					} else if(event_count < 0) {
						LOG->Warn("Problem waiting for write on stream: %s", std::strerror(errno));
						seenError = true;
						return false;
					} else if(FD_ISSET(ph, &set)) {
						// There may now be room on our stream
						ssize_t count = write(ph, data, remaining);
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

			bool awaitPendingOp(OVERLAPPED& overlapped, size_t timeout)
			{
				bool isPending = true;

				while(isPending) {
					isPending = false;

					if(!IsReady()) {
						return false;
					}

					DWORD waitResult = WaitForSingleObject(overlapped.hEvent, (DWORD) timeout);

					switch(waitResult) {
						case WAIT_OBJECT_0: {
							// Finished
							BOOL overlappedResult = GetOverlappedResult(ph, &overlapped, NULL, FALSE);

							if(overlappedResult) {
								// Op completed successfully
								ResetEvent(overlapped.hEvent);
								return true;
							} else {
								RString msg = GetWindowsErrorMessage(GetLastError());
								LOG->Warn("GetOverlappedResult failed: %s", msg.c_str());
								return false;
							}
						}
						case WAIT_TIMEOUT: {
							// Timeout expired

							// Stop if no longer ready
							if(!IsReady()) {
								return false;
							}

							// Else, continue waiting
							isPending = true;
							break;
						}
						default: {
							RString msg = GetWindowsErrorMessage(GetLastError());
							LOG->Warn("WaitForSingleObject failed: %s", msg.c_str());
							return false;
						}
					}
				}
			}

			bool putData0(OVERLAPPED& overlapped, const void * data, size_t length)
			{
				BOOL writeFileResult = WriteFile(ph, data, length, NULL, &overlapped);

				if(writeFileResult) {
					// Write was performed synchronously
					return true;
				} else {
					DWORD writeFileError = GetLastError();

					switch(writeFileError) {
						case ERROR_HANDLE_EOF:
							seenEof = true;
							LOG->Warn("Cannot write to pipe; EOF found");
							return false;
						case ERROR_IO_PENDING:
							if(awaitPendingOp(overlapped, TIMEOUT)) {
								// Success
								return true;
							} else {
								LOG->Warn("Write error reported while waiting");
								seenError = true;
								return false;
							}
						default: {

							RString msg = GetWindowsErrorMessage(writeFileError);
							LOG->Warn("WriteFile to pipe failed: %s", msg.c_str());
							seenError = true;
							return false;
						}
					}
				}
			}

			bool putData(const void * data, size_t length)
			{
				OVERLAPPED overlapped;
				ZeroMemory(&overlapped, sizeof(overlapped));
				overlapped.hEvent = CreateEventForOverlapped();
				if(overlapped.hEvent == NULL) {
					return false;
				}

				bool result = putData0(overlapped, data, length);

				CloseHandle(overlapped.hEvent);

				return result;
			}

		public:
			Impl(const RString& filename) : WritingLock("WritingLock")
			{
				stopRequested = false;

				LOG->Info(
					"Sextets Windows overlapped named pipe packet writer opening stream from '%s' for output", filename.c_str());

				ph = OpenNamedPipe(filename, BUSY_PIPE_TIMEOUT);
				seenEof = false;
				seenError = false;

				if(!HasStream()) {
					LOG->Warn(
						"Sextets Windows overlapped named pipe packet writer could not open stream from '%s' for output", filename.c_str());
					return;
				}
			}

			~Impl()
			{
				Close();
			}

			bool HasStream()
			{
				return ph != INVALID_HANDLE_VALUE;
			}

			bool IsReady()
			{
				return HasStream() && !stopRequested && !seenEof && !seenError;
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

				// Close ph and mark closed
				if(ph != INVALID_HANDLE_VALUE) {
					CloseHandle(ph);
					ph = INVALID_HANDLE_VALUE;
				}

				// Release lock
				WritingLock.Unlock();
			}

	};
}

namespace Sextets
{
	namespace IO
	{
		namespace Windows
		{
			NamedFifoPacketWriter* NamedFifoPacketWriter::Create(const RString& filename)
			{
				Impl * impl = new Impl(filename);
				return impl->HasStream() ? impl : NULL;
			}

			NamedFifoPacketWriter::~NamedFifoPacketWriter() {}
		}
	}
}

#endif // defined(SEXTETS_HAVE_WINDOWS)

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

