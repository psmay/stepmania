
#ifndef Sextets_IO_EzSocketsLineReader_h
#define Sextets_IO_EzSocketsLineReader_h

#include "global.h"
#include "arch/Sextets/IO/LineReader.h"

#if !defined(WITHOUT_NETWORKING)

namespace Sextets
{
	namespace IO
	{
		class EzSocketsLineReader : public LineReader
		{
			public:
				static EzSocketsLineReader * Create(const RString& host, unsigned short port);
		};

		/*
		class EzSocketsLineReaderFactory : public LineReaderFactory
		{
			private:
				const RString host;
				const unsigned short port;

			public:
				EzSocketsLineReaderFactory(const RString& host, unsigned short port);
		};
		*/
	}
}

#endif // !defined(WITHOUT_NETWORKING)

#endif
