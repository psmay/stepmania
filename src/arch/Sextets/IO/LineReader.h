
#ifndef Sextets_IO_LineReader_h
#define Sextets_IO_LineReader_h

#include "global.h"

namespace Sextets
{
	namespace IO
	{
		/**
		 * @brief Abstract interface for objects that produce lines of
		 * characters.
		 */
		class LineReader
		{
			public:
				/** @brief The default value for the `msTimeout` parameter
				 * of `ReadLine()`. */
				static const size_t DEFAULT_TIMEOUT_MS = 1000;

				LineReader()
				{
				}

				virtual ~LineReader()
				{
				}

				/**
				 * @brief Attempts to read the next line from this LineReader.
				 *
				 * Ideally, this method should return if msTimeout passes
				 * before a line becomes available. Actually doing this may
				 * require some platform-specific blocking-with-timeout read
				 * capability. This sort of thing could be implemented using
				 * e.g. POSIX select() and Windows WaitForSingleObject(),
				 * both of which have timeout parameters. (I get the sense
				 * that the RageFileDriverTimeout class could be convinced
				 * to work, but there isn't a lot of code using it, so I'm
				 * lacking the proper examples.)
				 *
				 * If this method does block indefinitely, almost everything
				 * will still work, but the blocking may prevent the loop
				 * from checking continueInputThread in a timely fashion. If
				 * the stream ceases to produce new data before this object
				 * is destroyed, the current thread will hang until the
				 * other side of the connection closes the stream (or
				 * produces a line of data). A workaround for that would be
				 * to have the far side of the connection repeat its last
				 * line every second or so as a keepalive.
				 *
				 * @retval true Success, with line set to the input line.
				 * @retval false No line was immediately available. The
				 *         caller should retry if and only if `IsValid()`
				 *         returns true.
				 */
				virtual bool ReadLine(RString& line, size_t msTimeout = DEFAULT_TIMEOUT_MS) = 0;

				/**
				 * @brief Checks whether this LineReader is still possibly
				 * readable.
				 * 
				 * False is returned if it is known that no future read from
				 * this LineReader will succeed. Reasons include that the
				 * input source has reached its end, been closed, or
				 * encountered an unrecoverable error. (The LineReader
				 * itself may dispose of its input source as soon as one of
				 * these issues has been detected or as late as the
				 * destructor.)
				 *
				 * True is returned when there is no condition that
				 * guarantees that a read will fail. This is not a promise
				 * that it necessarily will succeed before this reader goes
				 * invalid.
				 *
				 * @retval true A future read from this reader may still succeed.
				 * @retval false No future read from this reader can succeed.
				 */
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
//KWH
