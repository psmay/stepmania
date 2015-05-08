
#include "global.h"

#if !defined(WITHOUT_NETWORKING)

#include "arch/Sextets/IO/EzSocketsLineReader.h"
#include "arch/Sextets/IO/LineBuffer.h"
#include "arch/Sextets/Threads/MutexNames.h"
#include "RageLog.h"
#include "RageThreads.h"
#include "ezsockets.h"

namespace
{
	class _EzSocketsLineReader: public Sextets::IO::EzSocketsLineReader
	{
		private:
			// Note: After its initialization to true, allowMoreInput may
			// only be set to false.
			volatile bool allowMoreInput;

			EzSockets * volatile sock;
			Sextets::IO::LineBuffer * lines;

			static const size_t BUFFER_SIZE = 16;
			char buffer[BUFFER_SIZE];

			RageMutex mutex;

			inline void lock() { mutex.Lock(); }
			inline void unlock() { mutex.Unlock(); }

			inline bool socketIsConnected() {
				return (sock != NULL) && (sock->state != EzSockets::skDISCONNECTED);
			}

			inline void shutdown(bool fromDestructor = false)
			{
				// Cue ReadLine to stop looping
				allowMoreInput = false;

				// Wait for loop to stop before continuing
				lock();
				if(sock != NULL) {
					if(socketIsConnected()) {
						sock->close();
					}
					delete sock;
					sock = NULL;

					// If the shutdown is from the destructor, don't bother
					// making the remaining buffer data available as lines.
					if(!fromDestructor) {
						lines->Flush();
					}
				}
				unlock();
			}

			// Called from the loop if no data or zero-length data was
			// returned from the most recent read.
			inline bool shouldReadSocket()
			{
				return socketIsConnected() && !sock->IsError();
			}

			inline bool nosyncReadLine(RString& line, size_t msTimeout)
			{
				// Read out a buffered line first, if any
				if(lines->Next(line)) {
					return true;
				}

				// Read some data into the buffer.
				// If a line is produced, read it out.
				// If not, return false, but don't disallow more input
				// unless there was a problem.
				if(allowMoreInput && shouldReadSocket()) {
					size_t readLen = sock->DataAvailable(msTimeout) ?
						sock->ReadData(buffer, sizeof(buffer)) : 0;

					if(readLen > 0) {
						if(lines->AddData(buffer, readLen)) {
							// The new data ended a line
							lines->Next(line);
							return true;
						}
						else {
							// No line to return, but keep reader open
							return false;
						}
					}
					else {
						// No data was read.
						// Is there a problem, or do we just need to
						// wait longer?
						if(shouldReadSocket())
						{
							// No data added, but keep reader open
							return false;
						}
						else
						{
							// There shall be no further input.
							// Close() closes the socket, if open,
							// flushes the LineBuffer, and disallows new
							// input.
							Close();

							// The flush may have produced a line.
							// If so, read it out and return true.
							// Otherwise, no line, so return false.
							// Either way, IsValid() will return false
							// afterward.
							return lines->Next(line);
						}
					}
				}
				else {
					// Cannot buffer any new data.
					return false;
				}
			}

		public:
			_EzSocketsLineReader(EzSockets * sock) :
				lines(Sextets::IO::LineBuffer::Create()),
				allowMoreInput(true),
				mutex(Sextets::Threads::MutexNames::Make("EzSocketsLineReader", this)),
				sock(sock)
			{
			}

			virtual ~_EzSocketsLineReader()
			{
				shutdown(true);
				delete lines;
			}

			virtual bool ReadLine(RString& line, size_t msTimeout)
			{
				bool result;
				lock();
				result = nosyncReadLine(line, msTimeout);
				unlock();
				return result;
			}

			virtual bool IsValid()
			{
				return lines->HasNext() || lines->HasPartialLine() ||
					(allowMoreInput && shouldReadSocket());
			}

			virtual void Close()
			{
				shutdown();
			}
	};
}

namespace Sextets
{
	namespace IO
	{
		EzSocketsLineReader * EzSocketsLineReader::Create(const RString& host, unsigned short port)
		{
			EzSockets * sock = new EzSockets;

			sock->close();
			sock->create();
			sock->blocking = false;

			if(!sock->connect(host, port)) {
				LOG->Warn("Could not connect to %s:%u for input", host.c_str(), (unsigned int)port);
				delete sock;
				return NULL;
			}

			return new _EzSocketsLineReader(sock);
		}
	}
}

#endif // !defined(WITHOUT_NETWORKING)

//KWH
