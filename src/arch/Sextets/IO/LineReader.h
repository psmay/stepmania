
#ifndef Sextets_IO_LineReader_h
#define Sextets_IO_LineReader_h

#include "global.h"

namespace Sextets
{
	namespace IO
	{
		class LineReader
		{
			protected:
				int timeout_ms;

			public:
				static const size_t DEFAULT_TIMEOUT_MS = 1000;

				LineReader()
				{
					timeout_ms = DEFAULT_TIMEOUT_MS;
				}

				virtual ~LineReader()
				{
				}

				virtual bool IsValid()
				{
					return false;
				}

				// Ideally, this method should return if timeout_ms passes
				// before a line becomes available. Actually doing this may
				// require some platform-specific non-blocking read capability.
				// This sort of thing could be implemented using e.g. POSIX
				// select() and Windows GetOverlappedResultEx(), both of which
				// have timeout parameters. (I get the sense that the
				// RageFileDriverTimeout class could be convinced to work, but
				// there isn't a lot of code using it, so I'm lacking the proper
				// examples.)
				//
				// If this method does block, almost everything will still work,
				// but the blocking may prevent the loop from checking
				// continueInputThread in a timely fashion. If the stream ceases
				// to produce new data before this object is destroyed, the
				// current thread will hang until the other side of the
				// connection closes the stream (or produces a line of data). A
				// workaround for that would be to have the far side of the
				// connection repeat its last line every second or so as a
				// keepalive.
				//
				// false (line undefined) if there is an error or EOF condition,
				// true (line = next line from stream) if a whole line is available,
				// true (line = "") if no error but still waiting for next line.
				virtual bool ReadLine(RString& line) = 0;
		};
	}
}

#endif
