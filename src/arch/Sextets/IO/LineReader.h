
#ifndef Sextets_IO_LineReader_h
#define Sextets_IO_LineReader_h

#include "global.h"

namespace Sextets
{
	namespace IO
	{
		class LineReader
		{
			public:
				static const size_t DEFAULT_TIMEOUT_MS = 1000;

				LineReader()
				{
				}

				virtual ~LineReader()
				{
				}

				// Ideally, this method should return if msTimeout passes
				// before a line becomes available. Actually doing this may
				// require some platform-specific blocking-with-timeout read
				// capability. This sort of thing could be implemented using
				// e.g. POSIX select() and Windows WaitForSingleObject(),
				// both of which have timeout parameters. (I get the sense
				// that the RageFileDriverTimeout class could be convinced
				// to work, but there isn't a lot of code using it, so I'm
				// lacking the proper examples.)
				//
				// If this method does block indefinitely, almost everything
				// will still work, but the blocking may prevent the loop
				// from checking continueInputThread in a timely fashion. If
				// the stream ceases to produce new data before this object
				// is destroyed, the current thread will hang until the
				// other side of the connection closes the stream (or
				// produces a line of data). A workaround for that would be
				// to have the far side of the connection repeat its last
				// line every second or so as a keepalive.
				//
				// Returns true on success, with line set to the input line.
				// Returns false if no line was available. When false is
				// returned, the caller should retry if and only if
				// IsValid() returns true.
				virtual bool ReadLine(RString& line, size_t msTimeout = DEFAULT_TIMEOUT_MS) = 0;

				// Returns true if it is still possible that a call to
				// ReadLine() will succeed. If false is returned, in
				// general, no future call to ReadLine() will succeed
				// because the input source has reached its end or
				// encountered an unrecoverable error. (Incidentally,
				// if either of these situations applies, the LineReader
				// may close and deallocate the underlying stream, though it
				// may also delay doing this until the destructor.)
				virtual bool IsValid() = 0;
		};

		/*
		class LineReaderFactory
		{
			public:
				LineReaderFactory() {}
				virtual ~LineReaderFactory() {}
				virtual bool IsValid() = 0;
				virtual LineReader * Create() = 0;
		};
		*/
	}
}

#endif
