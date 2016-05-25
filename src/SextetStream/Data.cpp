
#include "SextetStream/Data.h"
#include "RageLog.h"

namespace SextetStream
{
	namespace Data
	{
		bool IsValidSextetByte(uint8_t value)
		{
			return (value >= 0x30) && (value <= 0x6F);
		}

		uint8_t ClearArmor(uint8_t value)
		{
			return value & 0x3F;
		}

		RString TrimTrailingNewlines(const RString& str0)
		{
			RString str = str0;

			size_t found = str.find_last_not_of("\x0A\x0D");
			if(found != RString::npos) {
				str.erase(found + 1);
			} else {
				str.clear();
			}

			return str;
		}

		bool ConvertPacketToState(uint8_t * buffer, size_t bufferSize, const RString& packet0)
		{
			RString packet = TrimTrailingNewlines(packet0);

			size_t packetLen = packet.length();
			size_t packetIndex, bufferIndex;

			// Clear buffer
			bufferIndex = 0;
			memset(buffer, 0, bufferSize);

			LOG->Trace("ConvertPacketToState processing '%s' (packet size %u, buffer size %u)", packet.c_str(), (unsigned)packetLen, (unsigned)bufferSize);

			packetIndex = 0;


			// Skip initial excess bytes
			while(packetIndex < packetLen) {
				if(IsValidSextetByte(packet[packetIndex])) {
					break;
				}
				++packetIndex;
			}

			LOG->Trace("ConvertPacketToState skipped %u leading excess byte(s)", (unsigned)packetIndex);

			// Use bytes from here to next excess byte (or end)
			while((packetIndex < packetLen) && (bufferIndex < bufferSize)) {
				uint8_t b = packet[packetIndex];
				if(IsValidSextetByte(b)) {
					buffer[bufferIndex] = ClearArmor(b);
					bufferIndex++;
					packetIndex++;
				} else {
					LOG->Trace("ConvertPacketToState converted %u byte(s); stopped at non-sextet char 0x%02x", (unsigned)packetIndex, (unsigned)b);
					return true;
				}
			}

			if(packetIndex < packetLen) {
				LOG->Trace("ConvertPacketToState converted %u byte(s); output buffer full", (unsigned)packetIndex);
				return false;
			} else {
				LOG->Trace("ConvertPacketToState converted %u byte(s); packet fully read", (unsigned)packetIndex);
				return true;
			}
		}

		void XorBuffers(uint8_t * result, const uint8_t * a, const uint8_t * b, size_t size)
		{
			size_t i;
			for(i = 0; i < size; ++i) {
				result[i] = a[i] ^ b[i];
			}
		}

#define BIT_IN_BYTE_BUFFER(buffer, byteIndex, subBitIndex) (buffer[byteIndex] & (1 << subBitIndex))

		void ProcessChanges(const uint8_t * state, const uint8_t * changedBits, size_t bufferSize, size_t bitCount, void * context, void updateButton(void * context, size_t index, bool value))
		{
			for(size_t byteIndex = 0; byteIndex < bufferSize; ++byteIndex) {
				for(size_t subBitIndex = 0; subBitIndex < 6; ++subBitIndex) {
					size_t bitIndex = (byteIndex * 6) + subBitIndex;
					if(bitIndex < bitCount) {
						if(BIT_IN_BYTE_BUFFER(changedBits, byteIndex, subBitIndex)) {
							bool value = BIT_IN_BYTE_BUFFER(state, byteIndex, subBitIndex);
							updateButton(context, bitIndex, value);
						}
					} else {
						// bitCount reached
						break;
					}
				}
			}
		}
	}
}


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
