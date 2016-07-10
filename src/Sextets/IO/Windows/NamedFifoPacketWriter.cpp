
#include "Sextets/IO/Windows/NamedFifoPacketWriter.h"
#include "Sextets/IO/Windows/_BasicOverlappedNamedPipe.h"
#include "RageLog.h"
#include "RageThreads.h"

#if defined(SEXTETS_HAVE_WINDOWS)

namespace
{
	using namespace Sextets;
	using namespace Sextets::IO;
	using namespace Sextets::IO::Windows;

	class Impl : public Sextets::IO::Windows::NamedFifoPacketWriter
	{
		private:
			RageMutex lock;

		protected:
			_BasicOverlappedNamedPipe * stream;

		private:

			void close()
			{
				lock.Lock();
				if(stream != NULL) {
					delete stream;
					stream = NULL;
				}
				lock.Unlock();
			}

			bool isReadyNoSync()
			{
				return (stream != NULL) && stream->IsReady();
			}

			bool writePacketNoSync(const Packet& packet)
			{
				if(isReadyNoSync()) {
					RString line = packet.GetLine() + "\n";
					size_t len = line.length() * sizeof(RString::value_type);
					size_t writtenLen;
					return stream->Write(line.data(), len, writtenLen);
				}
				// Not ready
				return false;
			}

		public:
			Impl(const RString& filename) : lock("NamedFifoPacketWriter")
			{
				LOG->Info("Starting Sextets packet writer from named pipe with filename '%s'", filename.c_str());
				stream = _BasicOverlappedNamedPipe::Create(filename, false, true);

				if(stream == NULL) {
					LOG->Warn("Could not named pipe '%s' for output");
				} else {
					LOG->Info("Pipe opened");
				}
			}

			bool HasStream()
			{
				lock.Lock();
				bool result = stream != NULL;
				lock.Unlock();
				return result;
			}

			~Impl()
			{
				close();
			}

			virtual bool IsReady()
			{
				lock.Lock();
				bool result = isReadyNoSync();
				lock.Unlock();
				return result;
			}

			virtual bool WritePacket(const Packet& packet)
			{
				lock.Lock();
				bool result = writePacketNoSync(packet);
				lock.Unlock();
				return result;
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
				if(impl->HasStream()) {
					return impl;
				}
				delete impl;
				return NULL;
			}

			NamedFifoPacketWriter::~NamedFifoPacketWriter() {}

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