
#include "arch/Sextets/IO/StdCFileLineReader.h"
#include "RageLog.h"

namespace
{
	class _StdCFileLineReader: public Sextets::IO::StdCFileLineReader
	{
		private:
			// The buffer size isn't critical; the RString will simply be
			// extended until the line is done.
			static const size_t BUFFER_SIZE = 64;
			char buffer[BUFFER_SIZE];
		protected:
			std::FILE * file;

		public:
			_StdCFileLineReader(std::FILE * file)
			{
				this->file = file;
			}

			~_StdCFileLineReader()
			{
				if(file != NULL) {
					std::fclose(file);
				}
			}

			virtual bool IsValid()
			{
				return file != NULL;
			}

			virtual bool ReadLine(RString& line)
			{
				bool afterFirst = false;
				size_t len;

				line = "";

				if(file != NULL) {
					while(fgets(buffer, BUFFER_SIZE, file) != NULL) {
						afterFirst = true;
						line += buffer;
						len = line.length();
						if(len > 0 && line[len - 1] == 0xA) {
							break;
						}
					}
				}

				return afterFirst;
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
