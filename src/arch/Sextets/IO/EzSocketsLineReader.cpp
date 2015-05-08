
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
			// Note: After its initialization to true, keepRunning may only
			// be set to false.
			volatile bool keepRunning;

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
				keepRunning = false;

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
			inline bool shouldReadAgain()
			{
				return socketIsConnected() && !sock->IsError();
			}

			inline bool nosyncReadLine(RString& line, size_t msTimeout)
			{
				// Read out a buffered line first, if any
				if(lines->Next(line)) {
					return true;
				}

				// Read data into line buffer until a line appears
				{
					size_t readLen;

					while(keepRunning && socketIsConnected()) {
						readLen = sock->DataAvailable(msTimeout) ?
							sock->ReadData(buffer, sizeof(buffer)) : 0;

						if(readLen > 0) {
							if(lines->AddData(buffer, readLen)) {
								// The new data ended a line
								lines->Next(line);
								return true;
							}
						}
						else {
							// No data was read.
							// Is there a problem, or do we just need to
							// wait longer?
							if(!shouldReadAgain())
							{
								keepRunning = false;
							}
						}
					}

					// At this point, there shall be no further input.
					// The socket may still be open, so we close it here.
					shutdown();

					// shutdown() flushes any remaining buffered data as a
					// line. If such a line exists, we read it out here.
					return lines->Next(line);
				}
			}

		public:
			_EzSocketsLineReader(EzSockets * sock) :
				lines(Sextets::IO::LineBuffer::Create()),
				keepRunning(true),
				mutex(Sextets::Threads::MutexNames::Make("EzSocketsLineReader", this)),
				sock(sock)
			{
			}

			virtual ~_EzSocketsLineReader()
			{
				shutdown(true);
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
					(keepRunning && socketIsConnected());
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


