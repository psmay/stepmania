
#include "Sextets/IO/EzSocketsPacketWriter.h"

#if !defined(WITHOUT_NETWORKING)

#include "RageLog.h"
#include "RageUtil.h"
#include "RageThreads.h"

#include "ezsockets.h"

namespace
{
	using namespace Sextets;
	using namespace Sextets::IO;

	EzSockets * OpenSocket(const RString& host, unsigned short port)
	{
		EzSockets * sock = new EzSockets;
		sock->close();
		sock->create();

		// TODO: Figure out how to make this work without blocking (or
		// blocking with a brief timeout).
		sock->blocking = true;

		if(!sock->connect(host, port)) {
			delete sock;
			return NULL;
		}

		return sock;
	}

	static const unsigned int TIMEOUT_MS = 1000;

	class PwImpl : public EzSocketsPacketWriter
	{
	private:
		EzSockets * sock;

		bool shouldWriteSocket()
		{
			return (sock != NULL) && !sock->IsError();
		}

	public:
		PwImpl(EzSockets * sock) : sock(sock)
		{
		}

		PwImpl(const RString& host, unsigned short port)
		{
			LOG->Info("Sextets packet writer opening EzSockets connection to '%s:%u' for output", host.c_str(), (unsigned) port);

			sock = OpenSocket(host, port);

			if(sock == NULL) {
				LOG->Warn("Sextets packet writer could not open EzSockets connection to '%s:%u' for output", host.c_str(), (unsigned) port);
			}
		}

		bool HasSocket()
		{
			return sock != NULL;
		}

		void Close()
		{
			if(sock != NULL) {
				sock->close();
				delete sock;
				sock = NULL;
			}
		}

		~PwImpl()
		{
			Close();
		}

		bool IsReady()
		{
			return shouldWriteSocket();
		}

		bool WritePacket(const Packet& packet)
		{
			if(shouldWriteSocket()) {
				sock->SendData(packet.GetLine());
				return true;
			} else {
				Close();
			}
		}
	};
}

namespace Sextets
{
	namespace IO
	{
		EzSocketsPacketWriter* EzSocketsPacketWriter::Create(const RString& host, unsigned short port)
		{
			PwImpl * obj = new PwImpl(host, port);
			if(!obj->HasSocket()) {
				delete obj;
				return NULL;
			}
			return obj;
		}

		EzSocketsPacketWriter::~EzSocketsPacketWriter() {}
	}
}

#endif // !defined(WITHOUT_NETWORKING)

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
