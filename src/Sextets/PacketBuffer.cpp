
#include "Sextets/PacketBuffer.h"
#include "RageLog.h"
#include "RageUtil.h"

#include <queue>

typedef RString::value_type RChr;

namespace
{
	static const size_t LINE_MAX_LENGTH = 4096;

#define ASCII_NEWLINE_CHARS "\x0a\x0d"
#define ASCII_PRINTABLE_CHARS \
	"\x20\x21\x22\x23\x24\x25\x26\x27" \
	"\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f" \
	"\x30\x31\x32\x33\x34\x35\x36\x37" \
	"\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f" \
	"\x40\x41\x42\x43\x44\x45\x46\x47" \
	"\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f" \
	"\x50\x51\x52\x53\x54\x55\x56\x57" \
	"\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f" \
	"\x60\x61\x62\x63\x64\x65\x66\x67" \
	"\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f" \
	"\x70\x71\x72\x73\x74\x75\x76\x77" \
	"\x78\x79\x7a\x7b\x7c\x7d\x7e"
#define SIMPLE_CHARS ASCII_PRINTABLE_CHARS ASCII_NEWLINE_CHARS

	using namespace Sextets;

	inline bool EraseTo(RString& str, size_t index, size_t additional = 0)
	{
		if(index == RString::npos) {
			return false;
		}
		str.erase(0, index + additional);
		return true;
	}

	inline bool EraseToFirstOf(RString& str, const RChr * chars, size_t additional = 0)
	{
		size_t i = str.find_first_of(chars);
		return EraseTo(str, i, additional);
	}

	class ActualPacketBuffer : public PacketBuffer
	{
	private:
		std::queue<RString> lines;
		RString buffer;

		// If true, all data through the next newline will be discarded.
		// This is set where continuing a line after a mid-line error would
		// ruin the alignment of the data on the line.
		bool needsResync;

		void RequestResync()
		{
			needsResync = true;
			buffer.clear();
		}

		// If needsResync, attempts to find and erase through the first
		// newline character in the buffer. If successful, needsResync is
		// set to false. Returns whether the buffer is now synced (which is
		// !needsResync).
		bool SyncBuffer()
		{
			if(needsResync) {
				if(EraseToFirstOf(buffer, ASCII_NEWLINE_CHARS, 1)) {
					// Newline found; erased through newline to sync
					needsResync = false;
					return true;
				} else {
					// No newline
					buffer.clear();
					return false;
				}
			}
			return true;
		}

		void PushLine(const RString& line)
		{
			if(!line.empty()) {
				lines.push(line);
			}
		}

		// Splits the buffer into lines.
		void BreakBuffer()
		{
			while(!buffer.empty() && SyncBuffer()) {
				if(EraseToFirstOf(buffer, ASCII_PRINTABLE_CHARS)) {
					// Removed leading non-printable chars
				} else {
					// The whole buffer is non-printable
					buffer.clear();
					return;
				}

				size_t iNewline = buffer.find_first_of(ASCII_NEWLINE_CHARS);
				size_t iNonPrint = buffer.find_first_not_of(ASCII_PRINTABLE_CHARS);

				// The first non-printable should be the same as the
				// first newline.
				if(iNewline != iNonPrint) {
					// Non-printable, non-newline characters may appear in
					// the input, but they must not appear on the same line
					// as a printable character.
					RequestResync();
					LOG->Warn(
						"Sextets PacketBuffer encountered a "
						"non-newline, non-printable character at a "
						"position where it is not valid. The current "
						"line has been discarded.");

					continue;
				} else {
					size_t length = (iNewline == RString::npos) ? buffer.length() : iNewline;

					if(length > LINE_MAX_LENGTH) {
						// Push truncated line (it might actually be larger
						// than LINE_MAX_LENGTH due to the length not having
						// been checked until after the input string has
						// been appended to the buffer)
						PushLine(buffer.substr(0, LINE_MAX_LENGTH));

						RequestResync();
						LOG->Warn(
							"Sextets PacketBuffer encountered a line "
							"that is longer than the allowed maximum "
							"length (%d). The current line has been "
							"truncated.", (unsigned)LINE_MAX_LENGTH);

						continue;
					}

					if(iNewline == RString::npos) {
						// No newline yet.
						// Leave the buffer as-is.
						return;
					} else {
						// A newline character exists before the end of the
						// buffer.
						PushLine(buffer.substr(0, iNewline));
						buffer.erase(0, iNewline + 1);
						continue;
					}
				}
			}
		}

	public:
		ActualPacketBuffer() : needsResync(false)
		{
		}

		~ActualPacketBuffer()
		{
		}

		void Add(const RChr * data, size_t length)
		{
			buffer.append(data, length);
			BreakBuffer();
		}

		void Add(const RString& data)
		{
			buffer.append(data);
			BreakBuffer();
		}

		void Add(const uint8_t * data, size_t length)
		{
			size_t start = buffer.length();
			buffer.resize(start + length);
			for(size_t i = 0; i < length; ++i) {
				buffer[start + i] = (RChr)(data[i]);
			}
			BreakBuffer();
		}

		bool GetPacket(Packet& packet)
		{
			if(lines.empty()) {
				return false;
			}

			packet.SetToLine(lines.front());
			lines.pop();

			return true;
		}
	};
}

namespace Sextets
{
	PacketBuffer* PacketBuffer::Create()
	{
		return new ActualPacketBuffer();
	}

	PacketBuffer::~PacketBuffer() {}
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
