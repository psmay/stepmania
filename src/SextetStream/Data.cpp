
#include "SextetStream/Data.h"
#include "RageLog.h"
#include "LightsManager.h"

// Number of printable characters used to encode lights
static const size_t CABINET_SEXTET_COUNT = 1;
static const size_t CONTROLLER_SEXTET_COUNT = 6;

// Number of bytes to contain the full pack
static const size_t FULL_SEXTET_COUNT = CABINET_SEXTET_COUNT + (NUM_GameController * CONTROLLER_SEXTET_COUNT);

#define SEXTET_PART(n) ((n) & 0x3F)
#define SEXTET_PART_EQUALS_ZERO(n) (SEXTET_PART(n) == 0)
#define SEXTET_PARTS_EQUAL(a,b) SEXTET_PART_EQUALS_ZERO((a)^(b))

#define SWAP_VIA(tmp, a, b) { tmp = a; a = b; b = tmp; }

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
			return SEXTET_PART(value);
		}

		void TrimTrailingNewlines(RString& str)
		{
			size_t found = str.find_last_not_of("\x0A\x0D");
			if(found != RString::npos) {
				str.erase(found + 1);
			} else {
				str.clear();
			}
		}

		size_t IndexOfTrimmableNewlines(const RString& str)
		{
			// Find index of last non-newline char.
			size_t found = str.find_last_not_of("\x0A\x0D");

			if(found == RString::npos) {
				// No non-newline character was found
				return 0;
			} else {
				// Found the last non-newline character
				return found + 1;
			}
		}

		RString CleanPacketCopy(const RString& str)
		{
			size_t left = 0;
			size_t right;
			size_t end = IndexOfTrimmableNewlines(str);

			// Find first valid byte
			while(left < end) {
				if(IsValidSextetByte(str[left])) {
					break;
				}
				++left;
			}

			if(left > 0) {
				LOG->Trace("CleanPacketCopy skipped %u leading excess byte(s)", (unsigned)left);
			}

			// Find first invalid byte after that
			right = left;
			while(right < end) {
				if(!IsValidSextetByte(str[right])) {
					break;
				}
				++right;
			}

			if(right < end) {
				LOG->Trace("CleanPacketCopy truncated after %u byte(s); stopped at non-sextet char 0x%02x", (unsigned)right, (unsigned)(str[right]));
			}

			return str.substr(left, right - left);
		}

		void CleanPacket(RString& str)
		{
			str = CleanPacketCopy(str);
		}

		bool ConvertPacketToState(uint8_t * buffer, size_t bufferSize, const RString& packet0)
		{
			RString packet = packet0;
			TrimTrailingNewlines(packet);

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








		///////////////////////////

		// Encodes the low 6 bits of a byte as a printable, non-space ASCII
		// character (i.e., within the range 0x21-0x7E) such that the low 6 bits of
		// the character are the same as the input.
		uint8_t ApplyArmor(uint8_t data)
		{
			// Maps the 6-bit value into the range 0x30-0x6F, wrapped in such a way
			// that the low 6 bits of the result are the same as the data (so
			// decoding is trivial).
			//
			//	00nnnn	->	0100nnnn (0x4n)
			//	01nnnn	->	0101nnnn (0x5n)
			//	10nnnn	->	0110nnnn (0x6n)
			//	11nnnn	->	0011nnnn (0x3n)

			// Put another way, the top 4 bits H of the output are determined from
			// the top two bits T of the input like so:
			// 	H = ((T + 1) mod 4) + 3

			return ((data + (uint8_t)0x10) & (uint8_t)0x3F) + (uint8_t)0x30;
		}

		void ApplyArmor(RString& str)
		{
			for(RString::iterator it = str.begin(); it != str.end(); ++it) {
				*it = ApplyArmor(*it);
			}
		}

		// Packs 6 booleans into a 6-bit value
		inline uint8_t PackPlainSextet(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5)
		{
			return (uint8_t)(
					   (b0 ? 0x01 : 0) |
					   (b1 ? 0x02 : 0) |
					   (b2 ? 0x04 : 0) |
					   (b3 ? 0x08 : 0) |
					   (b4 ? 0x10 : 0) |
					   (b5 ? 0x20 : 0));
		}

		// Packs 6 booleans into a printable sextet
		inline uint8_t PackArmoredSextet(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5)
		{
			return ApplyArmor(PackPlainSextet(b0, b1, b2, b3, b4, b5));
		}

		// Packs the cabinet lights into a printable sextet and adds it to a buffer
		inline size_t AppendCabinetLights(uint8_t * buffer, const LightsState * ls)
		{
			buffer[0] = PackArmoredSextet(
							ls->m_bCabinetLights[LIGHT_MARQUEE_UP_LEFT],
							ls->m_bCabinetLights[LIGHT_MARQUEE_UP_RIGHT],
							ls->m_bCabinetLights[LIGHT_MARQUEE_LR_LEFT],
							ls->m_bCabinetLights[LIGHT_MARQUEE_LR_RIGHT],
							ls->m_bCabinetLights[LIGHT_BASS_LEFT],
							ls->m_bCabinetLights[LIGHT_BASS_RIGHT]);
			return CABINET_SEXTET_COUNT;
		}

		// Packs the button lights for a controller into 6 printable sextets and
		// adds them to a buffer
		inline size_t AppendControllerLights(uint8_t * buffer, const LightsState * ls, GameController gc)
		{
			// Menu buttons
			buffer[0] = PackArmoredSextet(
							ls->m_bGameButtonLights[gc][GAME_BUTTON_MENULEFT],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_MENURIGHT],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_MENUUP],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_MENUDOWN],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_START],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_SELECT]);

			// Other non-sensors
			buffer[1] = PackArmoredSextet(
							ls->m_bGameButtonLights[gc][GAME_BUTTON_BACK],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_COIN],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_OPERATOR],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_EFFECT_UP],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_EFFECT_DOWN],
							false);

			// Sensors
			buffer[2] = PackArmoredSextet(
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_01],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_02],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_03],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_04],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_05],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_06]);
			buffer[3] = PackArmoredSextet(
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_07],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_08],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_09],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_10],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_11],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_12]);
			buffer[4] = PackArmoredSextet(
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_13],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_14],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_15],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_16],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_17],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_18]);
			buffer[5] = PackArmoredSextet(
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_19],
							false,
							false,
							false,
							false,
							false);

			return CONTROLLER_SEXTET_COUNT;
		}

		inline size_t AppendAllLights(uint8_t * buffer, const LightsState* ls)
		{
			size_t index = 0;

			index += AppendCabinetLights(&(buffer[index]), ls);

			FOREACH_ENUM(GameController, gc) {
				index += AppendControllerLights(&(buffer[index]), ls, gc);
			}

			return index;
		}

		RString BytesToRString(const void * buffer, size_t sizeInBytes)
		{
			const char * charBuffer = (const char *) buffer;
			return RString(charBuffer, sizeInBytes);
		}


		RString BytesToArmoredRString(const void * buffer, size_t sizeInBytes)
		{
			RString dest = BytesToRString(buffer, sizeInBytes);
			ApplyArmor(dest);
			return dest;
		}

		RString GetLightsStateAsPacket(const LightsState* ls)
		{
			uint8_t buffer[FULL_SEXTET_COUNT];
			size_t len = AppendAllLights(buffer, ls);
			return BytesToRString(buffer, len);
		}



		inline bool BuffersEqualAsSextets(const char * a, size_t aLength, const char * b, size_t bLength)
		{
			const char * sbuf = a;
			const char * lbuf = b;
			size_t slen = aLength;
			size_t llen = bLength;

			if(slen >= llen) {
				// slen must be no longer than llen.
				// Swap the buffers and lengths before comparing.
				const char * tmpbuf;
				size_t tmplen;

				SWAP_VIA(tmpbuf, sbuf, lbuf);
				SWAP_VIA(tmplen, slen, llen);
			}


			size_t i = 0;

			// Compare the existing portions
			while(i < slen) {
				if(!SEXTET_PARTS_EQUAL(sbuf[i], lbuf[i])) {
					return false;
				}
				++i;
			}

			// If one buffer is longer, pretend the shorter buffer has
			// infinite trailing zeros
			while(i < llen) {
				if(!SEXTET_PART_EQUALS_ZERO(lbuf[i])) {
					return false;
				}
				++i;
			}

			return true;
		}

		bool RStringSextetsEqual(const RString& a, const RString& b)
		{
			return BuffersEqualAsSextets(a.data(), a.length(), b.data(), b.length());
		}



		///////////////////////////
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
