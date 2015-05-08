
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
				 * @brief Reads next line from this LineReader.
				 *
				 * On success, the next line from this LineReader is copied
				 * to `line` and true is returned.
				 *
				 * Otherwise, `line` is unmodified and false is returned.
				 * When this occurs, the caller should call `IsValid()` to
				 * determine whether the next line simply was not yet
				 * available (true) or whether this LineReader has become
				 * unreadable (false). If false, any read loop from this
				 * object should be stopped.
				 *
				 *
				 * Ideally, an implementation of this method should block,
				 * and should do so only until the first occurrence of one
				 * of the following events:
				 *
				 * - A line has been successfully read
				 * - msTimeout milliseconds have passed
				 * - Close() has been called, which interrupts blocking on
				 *   this thread
				 *
				 * An implementation that does not block may cause the
				 * program to busy-wait since ReadLine() will often be
				 * called within a tight loop. Nonblocking streams often
				 * have a corresponding blocking monitor function (such as
				 * POSIX `select()`, perhaps some combination of
				 * `WaitForMultipleObjectsEx()` and `GetOverlappedResult()`
				 * on Windows) which supports a timeout value and, in some
				 * way, a pseudo-interrupt (for POSIX, create a separate
				 * pipe using `pipe(2)` and allow `select()` to wait on both
				 * it and the input stream which `Close()` can write a byte
				 * to it to make the `select()` unblock; for Windows,
				 * `CreatePipe()` can probably do the same trick).
				 *
				 * An implementation that blocks indefinitely and
				 * uninterruptibly may usually still work, but may cause the
				 * program to hang while closing until a new line of input
				 * has been read. If this is the case, have the supplier of
				 * input data to repeat the most recent line if no changes
				 * have happened within the past second or so, in order for
				 * the LineReader to stop blocking long enough to detect
				 * that it must shut down.
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

				/**
				 * @brief Closes this LineReader to further input.
				 *
				 * This method may block until any pending internal reads in
				 * other threads have returned. New internal reads should be
				 * rejected effective immediately. If at all possible, a
				 * call to this method should interrupt any blocking of an
				 * internal read within ReadLine().
				 * 
				 * If this LineReader contains buffered lines, then even
				 * after calling Close(), IsValid() should return true and
				 * ReadLine() should succeed until the buffer has been
				 * exhausted.
				 */
				virtual void Close() = 0;
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
