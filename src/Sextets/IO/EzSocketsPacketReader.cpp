
#include "Sextets/IO/EzSocketsPacketReader.h"

#if !defined(WITHOUT_NETWORKING)

#include "Sextets/PacketBuffer.h"
#include "ezsockets.h"
#include "RageLog.h"

namespace
{
	EzSockets * OpenSocket(const RString& host, unsigned short port)
	{
		EzSockets * sock = new EzSockets;
		sock->close();
		sock->create();
		sock->blocking = false;

		if(!sock->connect(host, port)) {
			delete sock;
			return NULL;
		}

		return sock;
	}

	static const unsigned int TIMEOUT_MS = 1000;
}

namespace
{
	using namespace Sextets;
	using namespace Sextets::IO;

	class Impl: public EzSocketsPacketReader
	{
	private:
		// This buffer's size isn't critical since it is only used to
		// receive data from C APIs. The resulting data is copied
		// directly into a PacketBuffer; which will simply be extended
		// as needed.
		static const size_t BUFFER_SIZE = 64;
		char buffer[BUFFER_SIZE];
		PacketBuffer * const pb;

		EzSockets * sock;


		inline bool socketIsConnected()
		{
			return (sock != NULL) && (sock->state != EzSockets::skDISCONNECTED);
		}

		inline bool shouldReadSocket()
		{
			return socketIsConnected() && !sock->IsError();
		}

	protected:

	public:
		Impl(const RString& host, unsigned short port)
			: pb(PacketBuffer::Create())
		{
			LOG->Info("Sextets packet reader opening EzSockets connection to '%s:%u' for input", host.c_str(), (unsigned)port);

			sock = OpenSocket(host, port);

			if(sock == NULL) {
				LOG->Warn("Sextets packet reader could not open EzSockets connection to '%s:%u' for input", host.c_str(), (unsigned)port);
			}
		}

		bool HasSocket()
		{
			return (sock != NULL);
		}

		void Close()
		{
			pb->DiscardUnfinished();
			if(sock != NULL) {
				LOG->Info("Sextets packet reader closing EzSockets connection");
				sock->close();
				delete sock;
				sock = NULL;
			}
		}

		~Impl()
		{
			Close();
			delete pb;
		}

		virtual bool IsReady()
		{
			return shouldReadSocket() || pb->HasPacket();
		}

		virtual bool ReadPacket(Packet& packet)
		{
			// Ensure there isn't already a line in the packet buffer.
			// (Even if this object has been closed, there may still be
			// leftover unused data in the packet buffer.)
			if(pb->GetPacket(packet)) {
				// Retrieved a waiting line from the packet buffer
				return true;
			}

			if(!shouldReadSocket()) {
				packet.Clear();
				return false;
			}

			// sock->DataAvailable() and sock->ReadData() combine
			// here as a "slightly blocking" read. If there is no
			// data, it will wait up to timeout ms for data to come
			// in. If any data has been received, it is then read.
			// If no data has been received, the read length is 0.
			// Therefore, this ReadPacket() implementation returns
			// in a finite (and small) amount of time.
			size_t readLen = sock->DataAvailable(TIMEOUT_MS) ?
							 sock->ReadData(buffer, BUFFER_SIZE) :
							 0;

			if(readLen == 0) {
				// No read. Could indicate an empty read or a
				// problem.

				if(shouldReadSocket()) {
					// Empty + true indicates that a non-blocking read
					// hasn't received a line yet, but the stream is
					// still open.
					packet.Clear();
					return true;
				} else {
					// EOF or unrecoverable error.
					Close();
					return false;
				}
			} else {
				// Nonzero read; add to the packet buffer.
				pb->Add(buffer, readLen);
				// Now, see if a packet became available due to this
				// read.
				if(pb->GetPacket(packet)) {
					// Retrieved a waiting line from the packet buffer
					return true;
				}
			}
		}
	};
}


namespace Sextets
{
	namespace IO
	{
		EzSocketsPacketReader* EzSocketsPacketReader::Create(const RString& host, unsigned short port)
		{
			Impl * obj = new Impl(host, port);
			if(!obj->HasSocket()) {
				delete obj;
				return NULL;
			}
			return obj;
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
