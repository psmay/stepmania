#include "global.h"
#include "LightsDriver_AsciiSextetsToFile.h"

REGISTER_SOUND_DRIVER_CLASS(AsciiSextetsToFile);

inline uint8_t encode6BitsAsPrintable(uint8_t data)
{
	// Encodes the low 6 bits of a byte as a printable, non-space ASCII
	// character (i.e., within the range 0x21-0x7E) such that the low 6 bits of
	// the character are the same as the input.

	// Maps the low 6 bits of data into the range 0x30-0x6F, wrapped in such a
	// way that the low 6 bits of the result are the same as the data (so
	// decoding is trivial).
	//
	//	00nnnn	->	0100nnnn (0x4n)
	//	01nnnn	->	0101nnnn (0x5n)
	//	10nnnn	->	0110nnnn (0x6n)
	//	11nnnn	->	0011nnnn (0x3n)

	// The top 4 bits H of the output are determined from the top two bits T
	// of the input like so:
	// 	H = ((T + 1) mod 4) + 3
	
	return ((data + (uint8_t)0x10) & (uint8_t)0x3F) + (uint8_t)0x30;
}

LightsDriver_AsciiSextetsToFile::LightsDriver_AsciiSextetsToFile()
{
}

LightsDriver_AsciiSextetsToFile::~LightsDriver_AsciiSextetsToFile()
{
}

void LightsDriver_AsciiSextetsToFile::Set( const LightsState *ls )
{
}

/*
 * Copyright © 2014 Peter S. May
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
