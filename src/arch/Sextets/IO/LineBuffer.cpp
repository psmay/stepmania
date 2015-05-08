
#include "global.h"
#include <queue>

#include "arch/Sextets/IO/LineBuffer.h"

namespace
{
	const RString::value_type CR = 0x0D;
	const RString::value_type LF = 0x0A;
	const RString::value_type * CRLF = "\x0D\x0A";

	// Converts arbitrary char input into lines.
	// Not (meant to be) thread-safe; synchronize accesses as needed.
	class _Impl : public Sextets::IO::LineBuffer
	{
		private:
			bool sawCr;
			bool beganLine;
			RString * partialLine;
			queue<const RString *> lines;

			// Given a sequence of input characters, finds the first CR or
			// LF and assigns it to *found. The parts of the input to the
			// left and right of that character are assigned to *left and
			// *right, respectively, as newly allocated strings. Then, true
			// is returned.
			//
			// Any of left, right, or found may be NULL. If left or right is
			// NULL, the new string object is not created for that side.
			//
			// If the input string contains no CR or LF, *left, *found, and
			// *right remain unchanged and false is returned.
			static inline bool splitAtLineEnding(const RString& input,
				const RString ** left,
				RString::value_type * found,
				const RString ** right)
			{
				size_t result = input.find_first_of(CRLF);
				if(result == RString::npos) {
					return false;
				}
				else {
					if(left != NULL) {
						*left = new RString(input.substr(0, result));
					}
					if(found != NULL) {
						*found = input[result];
					}
					if(right != NULL) {
						*right = new RString(input.substr(result + 1));
					}
					return true;
				}
			}

			// Append value to line in progress, then delete value.
			inline void appendAndDelete(const RString * value) {
				if(value != NULL) {
					if(partialLine == NULL)
						partialLine = new RString(*value);
					else
						*partialLine += *value;

					delete value;
				}
			}

			// Enqueue value as an output line.
			inline void pushLines(const RString * value) {
				if(value != NULL) {
					lines.push(value);
				}
			}

			// Dequeue an output line.
			inline const RString * shiftLines() {
				if(lines.empty()) {
					return NULL;
				}
				const RString * item = lines.front();
				lines.pop();
				return item;
			}

			// Move the line in progress to the output queue, regardless of
			// whether a line ending was found. (If there is no line in
			// progress, this is a no-op.)
			inline void flush() {
				if(partialLine != NULL) {
					pushLines(partialLine);
					partialLine = NULL;
				}
			}

			// Discard the line in progress, if it exists.
			inline void clearPartialLine() {
				if(partialLine != NULL) {
					delete partialLine;
					partialLine = NULL;
				}
			}

			// Discard all lines in the output queue.
			inline void clearLines() {
				const RString * item;
				while((item = shiftLines()) != NULL) {
					delete item;
				}
			}

			// Buffer a sequence of input characters, moving lines to the
			// lines queue as they are found.
			bool addData(const RString * topic) {
				const RString * left;
				const RString * right;
				RString::value_type found;

				while(splitAtLineEnding(*topic, &left, &found, &right)) {
					delete topic;
					topic = right;

					if(sawCr && left->empty() && found == LF) {
						// Discard LF after CR
						sawCr = false;
						delete left;
					}
					else {
						sawCr = (found == CR);
						appendAndDelete(left);
						flush();
					}
				}

				if(topic->empty()) {
					// This is not appended so blank lines are not counted
					// double. (Blank lines are counted when their line
					// endings are encountered.)
					delete topic;
				}
				else {
					appendAndDelete(topic);
				}

				return HasNext();
			}

		public:
			_Impl() :
				sawCr(false),
				partialLine(NULL)
			{
			}

			~_Impl() {
				clearPartialLine();
				clearLines();
			}

			bool AddData(const RString& data) {
				return addData(new RString(data));
			}

			bool AddData(const RString::value_type * data, size_t length) {
				return addData(new RString(data, length));
			}

			void Flush() {
				flush();
			}

			bool HasPartialLine() {
				return partialLine != NULL;
			}

			bool HasNext() {
				return !lines.empty();
			}

			const RString * Next() {
				return shiftLines();
			}

			bool Next(RString& line) {
				const RString * item = shiftLines();
				if(item == NULL) {
					return false;
				}
				else {
					line = *item;
					delete item;
					return true;
				}
			}
	};
}

namespace Sextets
{
	namespace IO
	{
		LineBuffer * LineBuffer::Create()
		{
			return new _Impl();
		}
	}
}
