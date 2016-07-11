
#include "Sextets/IO/PlatformFifo.h"

#if defined(SEXTETS_HAVE_WINDOWS)

#include "Sextets/IO/Windows/_BasicOverlappedNamedPipe.h"

#include <windows.h>
#include "RageLog.h"

namespace
{
	const size_t CYCLE_TIMEOUT = 1000;

	// Calls FormatMessageA() and stores the result in an RString.
	RString getErrorMessage(DWORD errorCode)
	{
		// FormatMessageA and LPSTR are used because RString is (as of this comment)
		// defined as a string of char, not wchar_t, and that this fact is not contingent
		// on `UNICODE` being defined.

		DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
					  FORMAT_MESSAGE_FROM_SYSTEM |
					  FORMAT_MESSAGE_IGNORE_INSERTS;

		DWORD langId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

		LPSTR bufferPtr = NULL;

		FormatMessageA(flags, NULL, errorCode, langId, (LPSTR)&bufferPtr, 0, NULL);
		RString message = RString((LPCSTR)bufferPtr);
		LocalFree(bufferPtr);

		return message;
	}

	// Same as getErrorMessage(GetLastError()).
	RString getLastErrorMessage()
	{
		return getErrorMessage(GetLastError());
	}


	// Opens an overlapping handle for an existing pipe.
	// This does not wait for a busy pipe; it must be rerun to try again.
	HANDLE openOverlappedExistingPipe(const RString& pipePath, bool forRead, bool forWrite)
	{
		DWORD desiredAccess = (forRead ? GENERIC_READ : 0) | (forWrite ? GENERIC_WRITE : 0);
		LPCSTR pipePathA = pipePath.c_str();

		HANDLE ph = CreateFileA(pipePathA, desiredAccess, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

		if(ph == INVALID_HANDLE_VALUE) {
			LOG->Warn("Could not open pipe '%s': %s", pipePathA, getLastErrorMessage().c_str());
		}

		return ph;
	}

	HANDLE createOverlappedEvent()
	{
		HANDLE event = CreateEvent(NULL, TRUE, FALSE, NULL);

		if(event == NULL) {
			LOG->Warn("Event handle for OVERLAPPED struct could not be created: %s", getLastErrorMessage());
			return INVALID_HANDLE_VALUE;
		}

		return event;
	}

	bool setUpOverlapped(OVERLAPPED& overlapped)
	{
		ZeroMemory(&overlapped, sizeof(OVERLAPPED));
		overlapped.hEvent = createOverlappedEvent();
		return overlapped.hEvent != INVALID_HANDLE_VALUE;
	}

	void shutDownOverlapped(OVERLAPPED& overlapped)
	{
		HANDLE event = overlapped.hEvent;
		overlapped.hEvent = INVALID_HANDLE_VALUE;

		if(event != INVALID_HANDLE_VALUE) {
			CloseHandle(event);
		}
	}


	class IoOp
	{
		public:
			virtual BOOL run(HANDLE pipeHandle, OVERLAPPED& overlapped) = 0;
	};

	class ReadFileIoOp : public IoOp
	{
		private:
			LPVOID buffer;
			DWORD bufferLength;

		public:
			ReadFileIoOp(LPVOID buffer, DWORD bufferLength) :
				buffer(buffer),
				bufferLength(bufferLength)
			{}

			BOOL run(HANDLE ph, OVERLAPPED& overlapped)
			{
				return ReadFile(ph, buffer, bufferLength, NULL, &overlapped);
			}
	};

	class WriteFileIoOp : public IoOp
	{
		private:
			LPCVOID data;
			DWORD dataLength;

		public:
			WriteFileIoOp(LPCVOID data, DWORD dataLength) :
				data(data),
				dataLength(dataLength)
			{}

			BOOL run(HANDLE ph, OVERLAPPED& overlapped)
			{
				return WriteFile(ph, data, dataLength, NULL, &overlapped);
			}
	};

	// Interlocked bool that starts false, can transition false->true but not true->false.
	class RaiseOnlyBool
	{
		private:
			volatile char charValue;
		public:
			RaiseOnlyBool() : charValue(0) {}

			bool get()
			{
				return (InterlockedOr8(&charValue, 0) != 0);
			}

			bool raise()
			{
				char oldValue = InterlockedOr8(&charValue, 0xff);
				return (oldValue != 0);
			}
	};

	class Impl : public Sextets::IO::Windows::_BasicOverlappedNamedPipe
	{
		private:
			RString pipePath;
			HANDLE ph;
			OVERLAPPED mix;

			RaiseOnlyBool seenError, seenEof, shutdownRequested;

			// If any flags are seen that make this object unusable, optionally
			// cancels a pending I/O operation on the handle using the indicated
			// OVERLAPPED, then closes this object (and its pipe) and returns
			// false.
			//
			// If this object is already closed, returns false.
			//
			// Otherwise, returns true.
			bool readyOrClose()
			{
				bool ready = false;

				if(!IsOpen()) {
					LOG->Info("Pipe is already closed");
				} else if(seenError.get()) {
					LOG->Warn("Closing pipe handle due to reported errors");
				} else if(seenEof.get()) {
					LOG->Warn("Closing pipe handle due to encountered EOF");
				} else if(shutdownRequested.get()) {
					LOG->Info("Closing pipe handle due to a cancellation request");
				} else {
					ready = true;
				}

				if(!ready) {
					HANDLE h = ph;
					if(h != INVALID_HANDLE_VALUE) {
						LOG->Info("Canceling any pending operation on pipe");
						CancelIoEx(h, &mix);
					}
					Close();
				}

				return ready;
			}


			// Returns whether an OVERLAPPED operation succeeded and, if so, gets the number of bytes transferred.
			bool checkOverlappedSuccess(DWORD& transferLengthOut)
			{
				if(!readyOrClose()) {
					return false;
				} else if(GetOverlappedResult(ph, &mix, &transferLengthOut, FALSE)) {
					// Operation successful
					ResetEvent(mix.hEvent);
					return true;
				} else {
					LOG->Warn("GetOverlappedResult returned failure; last error was: %s", getLastErrorMessage().c_str());
					return false;
				}
			}

			// Completes the rest of an OVERLAPPED operation that caused the error ERROR_IO_PENDING.
			bool completePendingOp(DWORD& transferLengthOut)
			{
				transferLengthOut = 0;

				for(;;) {
					if(!readyOrClose()) {
						return false;
					}

					DWORD waitResult = WaitForSingleObject(mix.hEvent, CYCLE_TIMEOUT);

					switch(waitResult) {
						case WAIT_OBJECT_0: {
							// Operation completed
							return checkOverlappedSuccess(transferLengthOut);
						}

						case WAIT_TIMEOUT: {
							// Operation still underway after the cycle timeout expired.
							// Cycle again, checking whether the continue condition is true.
							continue;
						}

						case WAIT_FAILED: {
							DWORD err = GetLastError();
							updateSeenFlags(err);
							LOG->Warn("WaitForSingleObject returned failure; last error was: %s", getErrorMessage(err).c_str());
						}

							// WAIT_ABANDONED only occurs if the HANDLE is for a mutex.
					}
				}
			}

			// Returns whether any flag was changed.
			bool updateSeenFlags(DWORD errorCode)
			{
				switch(errorCode) {
					default: {
						bool oldValue = seenError.raise();
						return !oldValue;
					}
					case ERROR_HANDLE_EOF: {
						bool oldValue = seenEof.raise();
						return !oldValue;
					}
					case ERROR_IO_PENDING:
						return false;
				}
			}

			// Interprets result and gets transfer length of an overlapped op.
			// Returns true if the loop should continue, or false if success contains the result.
			bool continueOverlappedIoOp(bool& success, DWORD opResult, const char* opName, const char* pipeName, DWORD& transferLengthOut)
			{
				if(opResult) {
					// Op was performed synchronously
					success = checkOverlappedSuccess(transferLengthOut);
					return false;
				} else {
					DWORD opError = GetLastError();
					updateSeenFlags(opError);

					if(opError == ERROR_IO_PENDING) {
						// Not an error
						if(completePendingOp(transferLengthOut)) {
							// Op succeeded
							success = true;
							return false;
						} else if(!readyOrClose()) {
							LOG->Warn("Pending %s operation on pipe '%s' stopped", opName, pipeName);
							success = false;
							return false;
						}
						// else, the call failed but the pipe can still be used
						return true;
					} else {
						LOG->Warn("Pipe %s operation failed: %s", opName, getErrorMessage(opError).c_str());
						success = false;
						return false;
					}
				}
			}

			bool doOverlappedIoOp(IoOp& ioOp, const char * opLabel, size_t& transferLengthOut)
			{
				DWORD transferLength = 0;
				bool success;

				while((success = false), readyOrClose()) {
					BOOL opResult = ioOp.run(ph, mix);

					if(!continueOverlappedIoOp(success, opResult, opLabel, pipePath.c_str(), transferLength)) {
						break;
					}
				}

				transferLengthOut = transferLength;
				return success;
			}



			bool readToLength(void * buffer, size_t bufferLength, size_t& receivedLengthOut)
			{
				ReadFileIoOp op(buffer, bufferLength);
				return doOverlappedIoOp(op, "read", receivedLengthOut);
			}

			bool getAvailableCount(size_t& availableLengthOut)
			{
				if(!readyOrClose()) {
					availableLengthOut = 0;
					return false;
				}

				DWORD available = 0;

				if(!PeekNamedPipe(ph, NULL, 0, NULL, &available, NULL)) {
					LOG->Warn("Peeking on pipe '%s' failed: %s", pipePath.c_str(), getLastErrorMessage().c_str());
					return false;
				}

				availableLengthOut = (size_t)available;
				return true;
			}




		public:
			Impl(const RString& pipePath, bool forRead, bool forWrite) :
				pipePath(pipePath)
			{
				ph = INVALID_HANDLE_VALUE;

				if(!setUpOverlapped(mix)) {
					LOG->Warn("Could not set up OVERLAPPED I/O event");
					return;
				}

				ph = openOverlappedExistingPipe(pipePath, forRead, forWrite);

				if(ph == INVALID_HANDLE_VALUE) {
					LOG->Warn("Could not open pipe handle");
					shutDownOverlapped(mix);
				}
			}

			bool IsOpen()
			{
				return ph != INVALID_HANDLE_VALUE;
			}

			bool IsReady()
			{
				// This must be identical to readyOrClose(), except that no closing and no log messages take place.
				return IsOpen() && !(seenError.get() || seenEof.get() || shutdownRequested.get());
			}

			void Close()
			{
				HANDLE h = ph;
				ph = INVALID_HANDLE_VALUE;

				if(h != INVALID_HANDLE_VALUE) {
					shutDownOverlapped(mix);
					CloseHandle(h);
				}
			}

			void RequestClose()
			{
				shutdownRequested.raise();
			}

			~Impl()
			{
				Close();
			}

			bool Write(const void * data, size_t dataLength, size_t& sentLengthOut)
			{
				if(!readyOrClose()) {
					return false;
				}

				if(dataLength <= 0) {
					// Trivial case
					return true;
				}

				WriteFileIoOp op(data, dataLength);
				return doOverlappedIoOp(op, "write", sentLengthOut);
			}

			// If any data is available immediately, read it. Else, do a read of
			// one byte.
			bool Read(void * buffer, size_t bufferLength, size_t& receivedLengthOut)
			{
				// Reasoning: While this may be paranoia, the way I read it, the
				// overlapped ReadFile() call is allowed to wait to complete
				// until the requested number of bytes has been read, a write
				// occurs on the far side of the pipe, or the call fails in
				// error, whichever happens first.
				//
				// The behavior to wait for a full buffer would be inconvenient
				// if, for example, if we have a buffer length of 64 but only 8
				// bytes are currently available, and the next write won't
				// happen for a nontrivial amount of time. 8 bytes is enough
				// room for multiple new packets, so we want to process them now
				// rather than wait until the next write. To remedy this, we
				// peek on the handle to determine how many bytes are available
				// *before* attempting to read. That way, we can request the
				// number of bytes available and retrieve them immediately.
				//
				// This workaround has its own problem, of course: The peeking
				// operation returns immediately; it does not have a timeout. If
				// there is no data available, the peek returns, the read is not
				// performed, and then we start over, creating a tight polling
				// loop with no delay. Here, we introduce a delay by requesting
				// 1 byte, so the attempted read operation does not complete
				// until *any* new data becomes available, but always completes
				// when it does, due to either the data becoming available, in
				// which case 1 byte is read, or to the write on the far side,
				// in which 0 bytes are (successfully) read. When a write on the
				// far side is greater than the 0 or 1 bytes read, which is the
				// typical case (especially on Windows, where a newline is 2
				// bytes), the remaining bytes from the write will be
				// immediately available for the next read.

				if(!readyOrClose()) {
					return false;
				}

				if(bufferLength <= 0) {
					// Trivial case
					return true;
				}

				size_t readLen = 0;
				if(!getAvailableCount(readLen)) {
					return false;
				}

				readLen = (readLen < bufferLength) ? readLen : bufferLength;

				if(readLen == 0) {
					// Nothing available; wait around until a byte appears
					readLen = 1;
				}

				return readToLength(buffer, readLen, receivedLengthOut);
			}
	};
}


namespace Sextets
{
	namespace IO
	{
		namespace Windows
		{
			_BasicOverlappedNamedPipe * _BasicOverlappedNamedPipe::Create(const RString& pipePath, bool forRead, bool forWrite)
			{
				Impl * impl = new Impl(pipePath, forRead, forWrite);
				if(!impl->IsOpen()) {
					delete impl;
					return NULL;
				}
				return impl;
			}
		}
	}
}

#endif // defined(SEXTETS_HAVE_WINDOWS)

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
