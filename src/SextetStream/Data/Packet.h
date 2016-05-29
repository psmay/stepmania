#ifndef SextetStream_Data_Packet_h
#define SextetStream_Data_Packet_h

#include "global.h"

// Needed for GetLightsStateAsPacket
struct LightsState;

namespace SextetStream
{
	namespace Data
	{
		class Packet
		{
		public:
			Packet();
			Packet(const Packet& packet);
			~Packet();

			void Clear();

			void Copy(const Packet& packet);

			// Sets this packet to the first span of valid sextet characters
			// within the string. All characters before the first valid
			// character, and all characters starting with the first invalid
			// character after the first valid character, are discarded.
			void SetToLine(const RString& line);

			// Calculates this XOR b, then assigns the result to this
			// packet.
			void SetToXor(const Packet& b);

			// Calculates a XOR b, then assigns the result to this packet.
			void SetToXor(const Packet& a, const Packet& b);

			typedef void (*ProcessEventCallback)(void * context,
												 size_t bitIndex, bool value);

			// Examines the low `bitCount` bits of `eventData` and, for each
			// `1` bit found, calls a callback with the bit index and value
			// of the bit at the same index of this packet.
			void ProcessEventData(const Packet& eventData, size_t bitCount,
								  void * context, ProcessEventCallback callback);

			// Gets whether this packet is equal to another packet if all
			// `0` bits are trimmed off the right of both.
			bool Equals(const Packet& b);

		private:
			class Impl;
			Impl * _impl;
		};
	}
}

#endif

/*
 * Copyright © 2016 Peter S. May
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
