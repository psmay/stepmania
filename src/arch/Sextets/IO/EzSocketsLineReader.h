
#ifndef Sextets_IO_EzSocketsLineReader_h
#define Sextets_IO_EzSocketsLineReader_h

#include "global.h"
#include "arch/Sextets/IO/LineReader.h"

#if !defined(WITHOUT_NETWORKING)

namespace Sextets
{
	namespace IO
	{
		/** @brief A LineReader that receives data from an EzSockets object.
		 */
		class EzSocketsLineReader : public LineReader
		{
			public:
				/** @brief Creates an EzSocketsLineReader that will connect
				 * to the given host and port.
				 */
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
//KWH
