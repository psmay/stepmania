#ifndef SEXTETSTREAM_DATA
#define SEXTETSTREAM_DATA

#include "global.h"

// Needed for GetLightsStateAsPacket
struct LightsState;

namespace SextetStream
{
	namespace Data
	{
		// Returns whether the given byte value falls within the valid
		// SextetStream data character range (0x30 .. 0x6F).
		bool IsValidSextetByte(uint8_t value);

		// Clears the top 2 bits of a byte.
		uint8_t ClearArmor(uint8_t value);

		// Alters the top 2 bits of a byte to produce a printable value in
		// 0x30 .. 0x6F.
		uint8_t ApplyArmor(uint8_t value);

		// Apply armor in-place to all characters of an RString.
		// (This does not trim excess characters first.)
		void ApplyArmor(RString& str);

		// Retrieves the first span of valid sextet characters
		// (`IsValidSextetByte()`) within the string. All characters before
		// the first valid character, and all characters starting with the
		// first invalid character after the first valid character, are
		// discarded.
		void CleanPacket(RString& str);
		RString CleanPacketCopy(const RString& str);

		// Given a raw packet, trims excess characters and removes armoring
		// bits, then places the result in a state byte array. Returns true
		// if the buffer was large enough for the entire packet, or false
		// otherwise.
		bool ConvertPacketToState(uint8_t * stateBuffer, size_t stateBufferSize, const RString& packet);

		// Performs XOR on a pair of byte arrays and writes the results to a
		// third. The destination array may be the same as either of the
		// source arrays.
		void XorBuffers(uint8_t * result, const uint8_t * a, const uint8_t * b, size_t size);

		// Examines the bits that have changed in a state and calls a
		// callback for each change.
		void ProcessChanges(const uint8_t * state, const uint8_t * changedBits, size_t bufferSize, size_t bitCount, void * context, void updateButton(void * context, size_t index, bool value));

		// Compares two RStrings and determines whether their contents would
		// be equal as sextet packets. The top two bits of each character
		// are discarded before comparing, but no excess characters are
		// trimmed. If one string is longer than the other, they are
		// compared as if both had infinite trailing zeros.
		bool RStringSextetsEqual(const RString& a, const RString& b);

		// Retrieves a sextet packet containing the information from a
		// LightsState.
		RString GetLightsStateAsPacket(const LightsState* ls);

		// Initializes an RString from the supplied buffer.
		RString BytesToRString(const void * buffer, size_t sizeInBytes);
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
