
#include "arch/Sextets/IO/StdCFileLineReader.h"
#include "arch/Sextets/IO/LineBuffer.h"
#include "arch/Sextets/Threads/MutexNames.h"

#include "RageLog.h"
#include "RageThreads.h"

namespace
{
	class _StdCFileLineReader: public Sextets::IO::StdCFileLineReader
	{
		private:
			typedef std::FILE Source;

			// Note: After its initialization to true, allowMoreInput may
			// only be set to false.
			volatile bool allowMoreInput;

			Source * volatile source;
			Sextets::IO::LineBuffer * lines;

			static const size_t BUFFER_SIZE = 16;
			char buffer[BUFFER_SIZE];

			RageMutex mutex;

			inline void lock() { mutex.Lock(); }
			inline void unlock() { mutex.Unlock(); }

			inline bool sourceIsLive() {
				return (source != NULL);
			}

			inline static void disposeSource(Source * p)
			{
				std::fclose(p);
				// Would delete here for a C++ object
			}

			inline void shutdown(bool fromDestructor = false)
			{
				// Cue ReadLine to stop looping
				allowMoreInput = false;

				// Wait for loop to stop before continuing
				lock();
				if(source != NULL) {
					Source * p = source;
					source = NULL;
					disposeSource(p);

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
			inline bool shouldReadSource()
			{
				return sourceIsLive() && !std::feof(source) && !std::ferror(source);
			}

			inline size_t readSource(char * buffer, size_t bufferSize, size_t msTimeout)
			{
				// timeout not supported
				return (std::fgets(buffer, bufferSize, source) != NULL) ?
					strnlen(buffer, bufferSize) : 0;
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
				if(allowMoreInput && shouldReadSource()) {
					size_t readLen = readSource(buffer, sizeof(buffer), msTimeout);

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
						if(shouldReadSource())
						{
							// No data added, but keep reader open
							return false;
						}
						else
						{
							// There shall be no further input.
							// Close() closes the source, if open,
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
			_StdCFileLineReader(Source * source) :
				lines(Sextets::IO::LineBuffer::Create()),
				allowMoreInput(true),
				mutex(Sextets::Threads::MutexNames::Make("StdCFileLineReader", this)),
				source(source)
			{
			}

			virtual ~_StdCFileLineReader()
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
					(allowMoreInput && shouldReadSource());
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

//KWH
