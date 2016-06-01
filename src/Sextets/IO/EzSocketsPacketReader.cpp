
#include "Sextets/IO/EzSocketsPacketReader.h"

#if !defined(WITHOUT_NETWORKING)

#include "ezsockets.h"
#include "RageLog.h"
#include "RageThreads.h"

namespace
{
	using namespace Sextets;
	using namespace Sextets::IO;

	class Impl: public EzSocketsPacketReader
	{
		private:
			// The buffer size isn't critical; the RString will simply be
			// extended until the packet is done.
			static const size_t BUFFER_SIZE = 64;
			char buffer[BUFFER_SIZE];

			RageMutex mutex;

			inline void lock() { mutex.Lock(); }
			inline void unlock() { mutex.Unlock(); }

			EzSockets * volatile sock;


			inline bool socketIsConnected() {
				return (sock != NULL) && (sock->state != EzSockets::skDISCONNECTED);
			}

			inline bool shouldReadSocket() {
				return socketIsConnected() && !sock->IsError();
			}

		protected:

		public:
			Impl(EzSockets * sock) : sock(sock)
			{
			}

			Impl(const RString& host, unsigned short port)
			{
				LOG->Info("Starting Sextets packet reader from ezsockets '%s:%u'",
					host.c_str(), (unsigned)port);

				EzSockets * sock = new 
				if(file == NULL) {
					LOG->Warn("Error opening file '%s' for input (cstdio): %s", filename.c_str(),
						std::strerror(errno));
				}
				else {
					LOG->Info("File opened");
					// Disable buffering on the file
					std::setbuf(file, NULL);
				}
			}


			~Impl()
			{
				if(file != NULL) {
					std::fclose(file);
				}
			}

			virtual bool IsReady()
			{
				return file != NULL;
			}

			virtual bool ReadPacket(Packet& packet)
			{
				bool afterFirst = false;

				if(file != NULL) {
					RString line;

					while(fgets(buffer, BUFFER_SIZE, file) != NULL) {
						afterFirst = true;
						line += buffer;
						size_t lineLength = line.length();
						if(lineLength > 0) {
							int lastChar = line[lineLength - 1];
							if(lastChar == 0xD || lastChar == 0xA) {
								break;
							}
						}
					}

					packet.SetToLine(line);
				}

				return afterFirst;
			}
	};
}

namespace Sextets
{
	namespace IO
	{
		EzSocketsPacketReader* EzSocketsPacketReader::Create(const RString& host, unsigned short port)
		{
			LOG->Info("Starting Sextets packet reader from ezsockets '%s:%u'", host.c_str(), (unsigned)port);

			EzSockets * sock = new EzSockets;
			sock->close();
			sock->create();
			sock->blocking = false;

			if(!sock->connect(host, port)) {
				LOG->Warn("Count not connect to %s:%u for input", host.c_str(), (unsigned)port);
				delete sock;
				return NULL;
			}

			return new Impl(sock);
		}
		
		EzSocketsPacketReader::~EzSocketsPacketReader() {}
	}
}

#endif // !defined(WITHOUT_NETWORKING)

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
