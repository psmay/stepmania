`InputHandler_SextetStream_*`
=============================

Explanation
-----------

This is a set of drivers (currently, just one driver) that accept button
inputs encoded in a stream of character. By accepting this data from a
stream, input can be produced by a separate program. Such a program can
be implemented using any language/platform that supports reading from
the desired input device (and writing to an output stream). If C++ isn't
your thing, or if learning the guts of StepMania seems a little much
just to implement an input driver, you're in the right place.

Quick start
-----------

You'll need a working StepMania build with the driver
`InputHandler_SextetStreamFromFile` enabled.

For this test, you'll run a script, `SextetStreamStdoutTest` (TODO:
provide this script), to verify that the driver is set up properly. The
script is a simple example of an *input program*, a program that will
receive input from some arbitrary input source, encode the button
states, and produce output to be read by StepMania. Because this script
is for testing and diagnostics, input is accepted on the console window
(or possibly stdin). Actual input programs produce output in the same
way, but will read input from different sources, such as data from a
hardware interface (serial/parallel/USB).

(TODO: This whole bit needs to be written.)

Encoding
--------

### Packing sextets

Values are encoded six bits at a time (hence "sextet"). The characters
are made printable, non-whitespace ASCII so that attempting to read the
data or pass it through a text-oriented channel will not cause problems.
The encoding is similar in concept to base64 or uuencode, but in this
scheme the low 6 bits remain unchanged.

Data is packed into the low 6 bits of a byte, then the two high bits are
set in such a way that the result is printable, non-whitespace ASCII:

    0x00-0x0F -> 0x40-0x4F
    0x10-0x1F -> 0x50-0x5F
    0x20-0x2F -> 0x60-0x6F
    0x30-0x3F -> 0x30-0x3F

(0x20-0x2F and 0x70-0x7F are avoided since they contain control or
whitespace characters.)

To encode a 6-bit value `s` in this way, this transform may be used:

    ((s + 0x10) & 0x3F) + 0x30

### Bit meanings

This driver produces events for a virtual game controller with no axes
and an arbitrary number of buttons. As with an actual gamepad, the
in-game meaning of each button can be configured in StepMania.

*   Byte 0
    *   0x01 Button #1
    *   0x02 Button #2
    *   0x04 Button #3
    *   0x08 Button #4
    *   0x10 Button #5
    *   0x20 Button #6
*   Byte 1
    *   0x01 Button #7
    *   0x02 Button #8
    *   0x04 Button #9
    *   0x08 Button #10
    *   0x10 Button #11
    *   0x20 Button #12
*   …
*   Byte `n`
    *   0x01 Button #`6n+1`
    *   0x02 Button #`6n+2`
    *   0x04 Button #`6n+3`
    *   0x08 Button #`6n+4`
    *   0x10 Button #`6n+5`
    *   0x20 Button #`6n+6`

A message is ended by terminating it with LF (0x0A) or CR LF (0x0D
0x0A). Data bytes outside 0x30-0x6F are discarded. The message can be as
large or as short as necessary, to encode all buttons. Any buttons not
encoded in a message are understood to have the value 0.

These messages both indicate that buttons 4 and 6 are pressed, and all
others are not:

    # Note: 0x68 & 0x3F == 0x28; 0x40 & 0x3F == 0x00
    0x68 0x40 0x40 0x0A
    0x68 0x0A

In the current implementation, all buttons after the first 64 are
ignored. (I hope you'll find you don't need that many.)

License
=======

Copyright © 2014 Peter S. May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
