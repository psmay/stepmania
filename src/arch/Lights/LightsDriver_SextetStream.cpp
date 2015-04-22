#include "global.h"
#include "LightsDriver_SextetStream.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "RageUtil.h"

#if !defined(WITHOUT_NETWORKING)
#include "ezsockets.h"
#endif

#include <cstring>

using namespace std;

// Number of printable characters used to encode lights
static const size_t CABINET_SEXTET_COUNT = 1;
static const size_t CONTROLLER_SEXTET_COUNT = 6;

// Number of bytes to contain the full pack
static const size_t FULL_SEXTET_COUNT = CABINET_SEXTET_COUNT + (NUM_GameController * CONTROLLER_SEXTET_COUNT);

// A funny way of saying "char"
typedef RString::value_type Sextet;

namespace
{
	// Abstract interface for line output
	class LineWriter
	{
	public:
		LineWriter() {}
		virtual ~LineWriter() {}

		// Returns true if the stream is open. If this method returns
		// false, the caller may assume that the stream is permanently
		// closed.
		virtual bool IsOpen() = 0;

		// Writes the line and a newline to output, and immediately flushes
		// (if applicable). If the stream is not open, this is a no-op.
		virtual void WriteLine(const RString& line) = 0;

		// If the underlying stream is closeable, flush and close the
		// stream. Calling this on a stream that is not open must be
		// harmless.
		virtual void Close() {}
	};

	// LineWriter for RageFile
	class RageFileLineWriter : public LineWriter
	{
	protected:
		RageFile * out;

	public:
		RageFileLineWriter(RageFile * file) {
			out = file;
		}

		virtual ~RageFileLineWriter() {
			Close();
		}

		virtual bool IsOpen() {
			return ((out != NULL) && out->IsOpen());
		}

		virtual void WriteLine(const RString& line) {
			if(IsOpen()) {
				RString lineNewl = line + "\n";
				out->Write(lineNewl);
			}
		}

		virtual void Close() {
			if(out != NULL) {
				if(out->IsOpen()) {
					out->Flush();
					out->Close();
				}
				out = NULL;
			}
		}
	};

#if !defined(WITHOUT_NETWORKING)
	// LineWriter for EzSockets
	class EzSocketsLineWriter : public LineWriter
	{
	protected:
		EzSockets * out;

	public:
		EzSocketsLineWriter(EzSockets * sock) {
			out = sock;
		}

		virtual ~EzSocketsLineWriter() {
			Close();
		}

		virtual bool IsOpen() {
			return ((out != NULL) && (out->state != EzSockets::skDISCONNECTED));
		}

		virtual void WriteLine(const RString& line) {
			out->SendData(line);
			out->SendData("\x0D\x0A");
		}

		virtual void Close() {
			if(IsOpen()) {
				out->close();
			}
			if(out != NULL) {
				delete out;
				out = NULL;
			}
		}
	};
#endif // !defined(WITHOUT_NETWORKING)
}


// Serialization routines

// Encodes the low 6 bits of a byte as a printable, non-space ASCII
// character (i.e., within the range 0x21-0x7E) such that the low 6 bits of
// the character are the same as the input.
inline Sextet printableSextet(uint8_t data)
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
inline uint8_t packSixBitByte(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5)
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
inline Sextet packPrintableSextet(bool b0, bool b1, bool b2, bool b3, bool b4, bool b5)
{
	return printableSextet(packSixBitByte(b0, b1, b2, b3, b4, b5));
}

// Packs the cabinet lights into a printable sextet
inline RString packCabinetLights(const LightsState *ls)
{
	RString buffer;
	buffer.reserve(CABINET_SEXTET_COUNT);

	buffer += packPrintableSextet(
		ls->m_bCabinetLights[LIGHT_MARQUEE_UP_LEFT],
		ls->m_bCabinetLights[LIGHT_MARQUEE_UP_RIGHT],
		ls->m_bCabinetLights[LIGHT_MARQUEE_LR_LEFT],
		ls->m_bCabinetLights[LIGHT_MARQUEE_LR_RIGHT],
		ls->m_bCabinetLights[LIGHT_BASS_LEFT],
		ls->m_bCabinetLights[LIGHT_BASS_RIGHT]);

	return buffer;
}

// Packs the button lights for a controller into 6 printable sextets
inline RString packControllerLights(const LightsState *ls, GameController gc)
{
	RString buffer;
	buffer.reserve(CONTROLLER_SEXTET_COUNT);

	// Menu buttons
	buffer += packPrintableSextet(
		ls->m_bGameButtonLights[gc][GAME_BUTTON_MENULEFT],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_MENURIGHT],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_MENUUP],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_MENUDOWN],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_START],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_SELECT]);

	// Other non-sensors
	buffer += packPrintableSextet(
		ls->m_bGameButtonLights[gc][GAME_BUTTON_BACK],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_COIN],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_OPERATOR],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_EFFECT_UP],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_EFFECT_DOWN],
		false);

	// Sensors
	buffer += packPrintableSextet(
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_01],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_02],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_03],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_04],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_05],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_06]);
	buffer += packPrintableSextet(
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_07],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_08],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_09],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_10],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_11],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_12]);
	buffer += packPrintableSextet(
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_13],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_14],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_15],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_16],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_17],
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_18]);
	buffer += packPrintableSextet(
		ls->m_bGameButtonLights[gc][GAME_BUTTON_CUSTOM_19],
		false,
		false,
		false,
		false,
		false);

	return buffer;
}

inline RString packFullMessage(const LightsState* ls)
{
	RString message;
	message.reserve(FULL_SEXTET_COUNT);

	message += packCabinetLights(ls);

	FOREACH_ENUM(GameController, gc)
	{
		message += packControllerLights(ls, gc);
	}

	return message;
}



// Private members/methods are kept out of the header using an opaque pointer `_impl`.
// Google "pimpl idiom" for an explanation of what's going on and why it is (or might be) useful.


// Implementation class

namespace
{
	class Impl
	{
	protected:
		RString lastOutput;
		LineWriter * lw;

	public:
		Impl(LineWriter * lineWriter) {
			lw = lineWriter;
			lastOutput.clear();
			lastOutput.reserve(FULL_SEXTET_COUNT);
		}

		virtual ~Impl() {
			if(lw != NULL) {
				delete lw;
				lw = NULL;
			}
		}

		void Set(const LightsState * ls)
		{
			RString message = packFullMessage(ls);

			// Only write if the message has changed since the last write.
			if(message != lastOutput)
			{
				if((lw != NULL) && lw->IsOpen())
				{
					lw->WriteLine(message);
				}

				// Remember last message
				lastOutput = message;
			}
		}
	};
}


// LightsDriver_SextetStream interface
// (Wrapper for Impl)

#define IMPL ((Impl*)_impl)

LightsDriver_SextetStream::LightsDriver_SextetStream()
{
	_impl = NULL;
}

LightsDriver_SextetStream::~LightsDriver_SextetStream()
{
	if(IMPL != NULL)
	{
		delete IMPL;
	}
}

void LightsDriver_SextetStream::Set(const LightsState *ls)
{
	if(IMPL != NULL)
	{
		IMPL->Set(ls);
	}
}


// LightsDriver_SextetStreamToFile implementation

REGISTER_SOUND_DRIVER_CLASS(SextetStreamToFile);

#if defined(_WINDOWS)
	#define DEFAULT_OUTPUT_FILENAME "\\\\.\\pipe\\StepMania-Lights-SextetStream"
#else
	#define DEFAULT_OUTPUT_FILENAME "Data/StepMania-Lights-SextetStream.out"
#endif
static Preference<RString> g_sSextetStreamOutputFilename("SextetStreamOutputFilename", DEFAULT_OUTPUT_FILENAME);

inline LineWriter * openOutputStream(const RString& filename)
{
	RageFile * file = new RageFile;

	if(!file->Open(filename, RageFile::WRITE|RageFile::STREAMED))
	{
		LOG->Warn("Error opening file '%s' for output: %s", filename.c_str(), file->GetError().c_str());
		SAFE_DELETE(file);
		file = NULL;
	}

	return new RageFileLineWriter(file);
}

LightsDriver_SextetStreamToFile::LightsDriver_SextetStreamToFile(RageFile * file)
{
	_impl = new Impl(new RageFileLineWriter(file));
}

LightsDriver_SextetStreamToFile::LightsDriver_SextetStreamToFile(const RString& filename)
{
	_impl = new Impl(openOutputStream(filename));
}

LightsDriver_SextetStreamToFile::LightsDriver_SextetStreamToFile()
{
	_impl = new Impl(openOutputStream(g_sSextetStreamOutputFilename));
}

#if !defined(WITHOUT_NETWORKING)
// LightsDriver_SextetStreamToSocket implementation

REGISTER_SOUND_DRIVER_CLASS(SextetStreamToSocket);

#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT 5734

static Preference<RString> g_sSextetStreamOutputSocketHost("SextetStreamOutputSocketHost", DEFAULT_HOST);
static Preference<int> g_sSextetStreamOutputSocketPort("SextetStreamOutputSocketPort", DEFAULT_PORT);

inline LineWriter * openOutputSocket(const RString& host, unsigned short port)
{
	EzSockets * sock = new EzSockets;

	sock->close();
	sock->create();
	sock->blocking = true;

	if(!sock->connect(host, port)) {
		LOG->Warn("Could not connect to host '%s' port %u for output", host.c_str(), (unsigned int)port);
		delete sock;
		sock = NULL;
	}

	return new EzSocketsLineWriter(sock);
}

LightsDriver_SextetStreamToSocket::LightsDriver_SextetStreamToSocket(EzSockets * sock)
{
	_impl = new Impl(new EzSocketsLineWriter(sock));
}

LightsDriver_SextetStreamToSocket::LightsDriver_SextetStreamToSocket(const RString& host, unsigned short port)
{
	_impl = new Impl(openOutputSocket(host, port));
}

LightsDriver_SextetStreamToSocket::LightsDriver_SextetStreamToSocket()
{
	RString host = g_sSextetStreamOutputSocketHost;
	int port = g_sSextetStreamOutputSocketPort;
	_impl = new Impl(openOutputSocket(host, (unsigned short)port));
}
#endif // !defined(WITHOUT_NETWORKING)

/*
 * Copyright © 2014-2015 Peter S. May
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
