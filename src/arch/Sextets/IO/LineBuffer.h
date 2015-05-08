
#ifndef Sextets_IO_LineBuffer_h
#define Sextets_IO_LineBuffer_h

namespace Sextets
{
	namespace IO
	{
		/** @brief Converts arbitrary char input into lines.
		 *
		 * Meant to be accessed by only one thread at a time; data is added
		 * using AddData() and then retrieved using Next(). New lines are
		 * made available whenever a newline is encountered in the input or
		 * anytime Flush() is called (which stuffs all buffered data to a
		 * new line).
		 */
		class LineBuffer
		{
			protected:
				LineBuffer() {};

			public:
				virtual ~LineBuffer() {};

				/** @brief Buffers characters from the given string. */
				virtual bool AddData(const RString& data) = 0;

				/** @brief Buffers characters from the given array. */
				virtual bool AddData(const RString::value_type * data, size_t length) = 0;

				/** @brief Uses the contents of the buffer, if any, as a new
				 * output line, and clears the buffer. */
				virtual void Flush() = 0;

				/** @brief Returns true if calling Flush() would cause a
				 * line to be available from Next(). */
				virtual bool HasPartialLine() = 0;

				/** @brief Returns true if there is at least one line
				 * available to be retrieved using Next(). */
				virtual bool HasNext() = 0;

				/** @brief Returns the next available line as a pointer.
				 *
				 * If a line is available, a pointer is returned and the
				 * caller is responsible for destroying the object. If no
				 * line is available, NULL is returned.
				 */
				virtual const RString * Next() = 0;

				/** @brief Retrieves a line, returning true if a line was
				 * available or false otherwise.
				 *
				 * If a line was available, the contents of the given string
				 * object are replaced with the line. Otherwise, the string
				 * object remains unchanged. No pointers change hands.
				 */
				virtual bool Next(RString& line) = 0;

				/** @brief Creates a new LineBuffer. */
				static LineBuffer * Create();
		};
	}
}

#endif
//KWH
