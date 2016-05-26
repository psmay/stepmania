#include "global.h"
#include "LightsDriver_SextetStream.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "RageUtil.h"

#include <cstring>
#include <string>

#include "SextetStream/IO/PacketWriter.h"
#include "SextetStream/IO/NoopPacketWriter.h"
#include "SextetStream/IO/RageFilePacketWriter.h"

using namespace std;

// Number of printable characters used to encode lights
static const size_t CABINET_SEXTET_COUNT = 1;
static const size_t CONTROLLER_SEXTET_COUNT = 6;

// Number of bytes to contain the full pack
static const size_t FULL_SEXTET_COUNT = CABINET_SEXTET_COUNT + (NUM_GameController * CONTROLLER_SEXTET_COUNT);

namespace SextetStream
{
	namespace Data
	{

		// Serialization routines

		// Encodes the low 6 bits of a byte as a printable, non-space ASCII
		// character (i.e., within the range 0x21-0x7E) such that the low 6 bits of
		// the character are the same as the input.
		uint8_t xxprintableSextet(uint8_t data)
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

		// Packs 6 booleans into a 6-bit value
		uint8_t xxpackPlainSextet(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5)
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
		uint8_t xxpackPrintableSextet(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5)
		{
			return xxprintableSextet(xxpackPlainSextet(b0, b1, b2, b3, b4, b5));
		}

		// Packs the cabinet lights into a printable sextet and adds it to a buffer
		size_t xxpackCabinetLights(uint8_t * buffer, const LightsState * ls)
		{
			buffer[0] = xxpackPrintableSextet(
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
		size_t xxpackControllerLights(uint8_t * buffer, const LightsState * ls, GameController gc)
		{
			// Menu buttons
			buffer[0] = xxpackPrintableSextet(
							ls->m_bGameButtonLights[gc][GAME_BUTTON_MENULEFT],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_MENURIGHT],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_MENUUP],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_MENUDOWN],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_START],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_SELECT]);

			// Other non-sensors
			buffer[1] = xxpackPrintableSextet(
							ls->m_bGameButtonLights[gc][GAME_BUTTON_BACK],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_COIN],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_OPERATOR],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_EFFECT_UP],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_EFFECT_DOWN],
							false);

			// Sensors
			buffer[2] = xxpackPrintableSextet(
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_01],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_02],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_03],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_04],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_05],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_06]);
			buffer[3] = xxpackPrintableSextet(
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_07],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_08],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_09],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_10],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_11],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_12]);
			buffer[4] = xxpackPrintableSextet(
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_13],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_14],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_15],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_16],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_17],
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_18]);
			buffer[5] = xxpackPrintableSextet(
							ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_19],
							false,
							false,
							false,
							false,
							false);

			return CONTROLLER_SEXTET_COUNT;
		}

		size_t xxpackAllLights(uint8_t * buffer, const LightsState* ls)
		{
			size_t index = 0;

			index += xxpackCabinetLights(&(buffer[index]), ls);

			FOREACH_ENUM(GameController, gc) {
				index += xxpackControllerLights(&(buffer[index]), ls, gc);
			}

			return index;
		}

		RString xxBytesToRString(const void * buffer, size_t sizeInBytes)
		{
			const char * charBuffer = (const char *) buffer;
			return RString(charBuffer, sizeInBytes);
		}

		RString GetLightsStateAsPacket(const LightsState* ls)
		{
			uint8_t buffer[FULL_SEXTET_COUNT];
			size_t len = xxpackAllLights(buffer, ls);
			return xxBytesToRString(buffer, len);
		}

		bool xxRStringsBinaryAndLengthEqual(const RString& a, const RString& b)
		{
			size_t len = a.length();

			if(b.length() != len) {
				return false;
			} else {
				return memcmp(a.c_str(), b.c_str(), len) == 0;
			}
		}

#define xxSEXTET_PART(n) ((n) & 0x3F)
#define xxSEXTET_PART_EQUALS_ZERO(n) (xxSEXTET_PART(n) == 0)
#define xxSEXTET_PARTS_EQUAL(a,b) xxSEXTET_PART_EQUALS_ZERO((a)^(b))

#define xxSWAP_VIA(tmp, a, b) { tmp = a; a = b; b = tmp; }

		bool xxBuffersEqualAsSextets(const char * a, size_t aLength, const char * b, size_t bLength)
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

				xxSWAP_VIA(tmpbuf, sbuf, lbuf);
				xxSWAP_VIA(tmplen, slen, llen);
			}


			size_t i = 0;

			// Compare the existing portions
			while(i < slen) {
				if(!xxSEXTET_PARTS_EQUAL(sbuf[i], lbuf[i])) {
					return false;
				}
				++i;
			}

			// If one buffer is longer, pretend the shorter buffer has
			// infinite trailing zeros
			while(i < llen) {
				if(!xxSEXTET_PART_EQUALS_ZERO(lbuf[i])) {
					return false;
				}
				++i;
			}

			return true;
		}

		bool RStringSextetsEqual(const RString& a, const RString& b)
		{
			return xxBuffersEqualAsSextets(a.c_str(), a.length(), b.c_str(), b.length());
		}


	}
}




// Implementation base class

class LightsDriver_SextetStream::Impl
{

private:
	RString previousPacket;
	SextetStream::IO::PacketWriter * writer;

public:
	Impl(SextetStream::IO::PacketWriter * writer)
	{
		if(writer == NULL) {
			writer = new SextetStream::IO::NoopPacketWriter();
		}
		this->writer = writer;

		// Clear the last output buffer
		previousPacket = "";
	}

	virtual ~Impl()
	{
		if(writer != NULL) {
			delete writer;
			writer = NULL;

			LOG->Info("Deleted writer");
			LOG->Flush();
		}
	}

	void Set(const LightsState * ls)
	{
		// Skip writing if the writer is not available.
		if(writer->IsReady()) {
			RString packet = SextetStream::Data::GetLightsStateAsPacket(ls);

			// Only write if the message has changed since the last write.
			if(!SextetStream::Data::RStringSextetsEqual(packet, previousPacket)) {
				writer->WritePacket(packet);
				LOG->Info("Packet: %s", packet.c_str());

				// Remember last message
				previousPacket = packet;
			}
		}
	}
};



// LightsDriver_SextetStream interface
// (Wrapper for Impl)

LightsDriver_SextetStream::LightsDriver_SextetStream()
{
	LOG->Info("Starting a SextetStream lights driver");
	_impl = NULL;
}

LightsDriver_SextetStream::~LightsDriver_SextetStream()
{
	LOG->Info("Destroying a SextetStream lights driver");
	LOG->Flush();
	if(_impl != NULL) {
		LOG->Info("Deleting an implementation");
		LOG->Flush();
		delete _impl;
		LOG->Info("Deleted implementation");
		LOG->Flush();
	}
}

void LightsDriver_SextetStream::Set(const LightsState *ls)
{
	_impl->Set(ls);
}


// LightsDriver_SextetStreamToFile implementation

REGISTER_LIGHTS_DRIVER_CLASS(SextetStreamToFile);

#if defined(_WINDOWS)
	#define DEFAULT_OUTPUT_FILENAME "\\\\.\\pipe\\StepMania-Lights-SextetStream"
#else
	#define DEFAULT_OUTPUT_FILENAME "Data/StepMania-Lights-SextetStream.out"
#endif
static Preference<RString> g_sSextetStreamOutputFilename("SextetStreamOutputFilename", DEFAULT_OUTPUT_FILENAME);

LightsDriver_SextetStreamToFile::LightsDriver_SextetStreamToFile()
{
	LOG->Info("Creating LightsDriver_SextetStreamToFile");
	LOG->Flush();

	SextetStream::IO::PacketWriter * writer =
		SextetStream::IO::RageFilePacketWriter::Create(g_sSextetStreamOutputFilename);

	if(writer == NULL) {
		LOG->Warn("Create of packet writer for LightsDriver_SextetStreamToFile failed.");
	} else {
		LOG->Info("Create of packet writer for LightsDriver_SextetStreamToFile OK.");
	}

	// Impl() accounts for the case where writer is NULL.
	_impl = new Impl(writer);
}

/*
 * Copyright © 2014-2016 Peter S. May
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
