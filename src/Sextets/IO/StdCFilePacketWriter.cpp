
#include "Sextets/IO/StdCFilePacketWriter.h"

#include "RageLog.h"
#include <cerrno>
#include <cstdio>

namespace
{
	using namespace Sextets;
	using namespace Sextets::IO;

	class Impl : public StdCFilePacketWriter
	{
	private:
		std::FILE * file;

	public:
		Impl(std::FILE * stream)
		{
			LOG->Info("Starting Sextets packet writer from open std::FILE");
			file = stream;
		}

		Impl(const RString& filename)
		{
			LOG->Info("Starting Sextets packet writer from std::FILE with filename '%s'",
					  filename.c_str());

			file = std::fopen(filename.c_str(), "wb");

			if(file == NULL) {
				LOG->Warn("Error opening file '%s' for output (cstdio): %s",
						  filename.c_str(), std::strerror(errno));
			} else {
				LOG->Info("File opened");
				// Disable buffering on the file
				std::setbuf(file, NULL);
			}
		}

		~Impl()
		{
			if(file != NULL) {
				std::fclose(file);
				file = NULL;
			}
		}

		bool InitOk()
		{
			return file != NULL;
		}

		bool IsReady()
		{
			return file != NULL;
		}

		bool WritePacket(const Packet& packet)
		{
			if(file != NULL) {
				RString line = packet.GetLine();
				std::fwrite(line.data(), sizeof(char), line.size(), file);
				std::fflush(file);

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
		StdCFilePacketWriter* StdCFilePacketWriter::Create(std::FILE * file)
		{
			Impl * impl = new Impl(file);
			return impl->InitOk() ? impl : NULL;
		}

		StdCFilePacketWriter* StdCFilePacketWriter::Create(const RString& filename)
		{
			Impl * impl = new Impl(filename);
			return impl->InitOk() ? impl : NULL;
		}

		StdCFilePacketWriter::~StdCFilePacketWriter() {}
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
