
#ifndef Sextets_IO_StdCFileLineReader_h
#define Sextets_IO_StdCFileLineReader_h

#include "global.h"
#include <cstdio>
#include "arch/Sextets/IO/LineReader.h"

namespace Sextets
{
	namespace IO
	{
		class StdCFileLineReader : public LineReader
		{
			public:
				// A file passed here will be de-buffered
				// (`setbuf(file,NULL)`). The file will be closed no later
				// than the deletion of this object.
				//
				// If a null std::FILE* is passed, no object is created and
				// NULL is returned.
				static StdCFileLineReader * Create(std::FILE * file);

				// If attempting to `fopen()` the given filename returns
				// NULL, no object is created and NULL is returned.
				static StdCFileLineReader * Create(const RString& filename);
		};

		/*
		class StdCFileLineReaderFactory : public LineReaderFactory
		{
			private:
				const RString filename;

			public:
				StdCFileLineReaderFactory(const RString& filename);
		};
		*/
	}
}

#endif
