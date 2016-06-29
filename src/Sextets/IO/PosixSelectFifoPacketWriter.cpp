

// THIS IS ONLY A PLACEHOLDER
// Don't expect this code to work (or even allow the entire program to work) in its current form.


#include "Sextets/IO/PosixSelectFifoPacketWriter.h"
#include "RageLog.h"
#include "RageUtil.h"

namespace
{
	using namespace Sextets;
	using namespace Sextets::IO;

	class PwImpl : public PosixSelectFifoPacketWriter
	{
	private:
		RageFile * out;

	public:
		PwImpl(RageFile * stream)
		{
			out = stream;
		}

		~PwImpl()
		{
			if(out != NULL) {
				out->Flush();
				out->Close();
				SAFE_DELETE(out);
			}
		}

		bool IsReady()
		{
			return out != NULL;
		}

		bool WritePacket(const Packet& packet)
		{
			if(out != NULL) {
				RString line = packet.GetLine();
				out->PutLine(line);
				out->Flush();

				return true;
			}
			return false;
		}
	};
}

namespace Sextets
{
	namespace IO
	{
		PosixSelectFifoPacketWriter* PosixSelectFifoPacketWriter::Create(const RString& filename)
		{
			RageFile * file = new RageFile;

			if(!file->Open(filename, RageFile::WRITE|RageFile::STREAMED)) {
				LOG->Warn("Error opening file '%s' for output: %s", filename.c_str(), file->GetError().c_str());
				SAFE_DELETE(file);
				return NULL;
			}

			return PosixSelectFifoPacketWriter::Create(file);
		}

		PosixSelectFifoPacketWriter* PosixSelectFifoPacketWriter::Create(RageFile * stream)
		{
			if(stream == NULL) {
				return NULL;
			}
			return new PwImpl(stream);
		}

		PosixSelectFifoPacketWriter::~PosixSelectFifoPacketWriter() {}
	}
}

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
