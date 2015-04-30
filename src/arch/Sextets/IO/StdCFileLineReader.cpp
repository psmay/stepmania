
#include "arch/Sextets/IO/StdCFileLineReader.h"
#include "RageLog.h"

namespace
{
	class _StdCFileLineReader: public Sextets::IO::StdCFileLineReader
	{
		private:
			std::FILE * file;

			static const size_t BUFFER_SIZE = 16;
			char buffer[BUFFER_SIZE];
			RString lineBuffer;
			bool lineStarted;

			void close()
			{
				if(file != NULL) {
					std::fclose(file);
					file = NULL;
				}
			}

			static const char CR = 0xD;
			static const char LF = 0xA;

			static inline bool isCrOrLf(char c)
			{
				return (c == CR) || (c == LF);
			}

			static bool chomp(RString& line)
			{
				size_t len = line.length();
				size_t lineEndingLength = 0;

				if((len >= 2) && (line[len - 2] == CR) && (line[len - 1] == LF)) {
					lineEndingLength = 2;
				}
				else if(len >= 1)
				{
					if(isCrOrLf(line[len - 1])) {
						lineEndingLength = 1;
					}
				}

				if(lineEndingLength > 0) {
					line.resize(len - lineEndingLength);
					return true;
				}
				else {
					return false;
				}
			}

			bool flushLineTo(RString& dest)
			{
				if(lineStarted) {
					dest = lineBuffer;
					lineBuffer = "";
					lineStarted = false;
					return true;
				}
				else {
					return false;
				}
			}


			bool isUnderlyingStreamLive()
			{
				if(file == NULL) {
					return false;
				}
				else if(std::feof(file) || std::ferror(file)) {
					close();
					return false;
				}
				return true;
			}

		public:
			_StdCFileLineReader(std::FILE * file) :
				file(file),
				lineStarted(false)
			{
			}

			~_StdCFileLineReader()
			{
				close();
			}

			virtual bool IsValid()
			{
				return lineStarted || isUnderlyingStreamLive();
			}

			virtual bool ReadLine(RString& line, size_t ignored_msTimeout)
			{
				line = "";

				if(!isUnderlyingStreamLive()) {
					// If the stream is dead, but there is data left in the
					// buffer, read it out here.
					return flushLineTo(line);
				}
				else if(fgets(buffer, BUFFER_SIZE, file) != NULL) {
					lineBuffer += buffer;
					lineStarted = true;

					if(chomp(lineBuffer)) {
						// A line ending was stripped, so the line has
						// finished being read.
						flushLineTo(line);
						return true;
					}
				}

				// If only a partial line was read, we return false here
				// just to give the caller some chance of terminating a loop
				// based on a flag. IsValid() will (probably) remain true,
				// telling the caller to try again.
				return false;
			}
	};
}

namespace Sextets
{
	namespace IO
	{
		StdCFileLineReader * StdCFileLineReader::Create(std::FILE * file)
		{
			if(file == NULL) {
				LOG->Info("Could not start StdCFileLineReader (file pointer was NULL)");
				return NULL;
			}

			LOG->Info("Starting StdCFileLineReader from open std::FILE");
			std::setbuf(file, NULL);
			return new _StdCFileLineReader(file);
		}

		StdCFileLineReader * StdCFileLineReader::Create(const RString& filename)
		{
			return Create(std::fopen(filename.c_str(), "rb"));
		}
	}
}
