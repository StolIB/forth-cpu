# Forth computing system

Author:

* Richard James Howe.

Copyright:

* Copyright 2013-2017 Richard James Howe.

License:

* MIT/LGPL

Email:

* howe.r.j.89@gmail.com

## Introduction

This project implements a small stack computer tailored to executing Forth
based on the [J1][] CPU. The processor has been rewritten in [VHDL][] from
[Verilog][], and extended slightly. The project is a work in progress and is
needs a lot of work before become usable. 

The goals of the project are as follows:

* Create a working version of [J1][] processor (called the H2).
* Make a working toolchain for the processor.
* Create a [FORTH][] for the processor which can take its input either from a
  [UART][] or a USB keyboard and a [VGA][] adapter.

The H2 processor, like the [J1][], is a stack based processor that executes an
instruction set especially suited for [FORTH][].

The current target is the [Nexys3][] board, with a [Xilinx][] Spartan-6 XC6LX16-CS324 
[FPGA][], new boards will be targeted in the future as this board is reaching it's 
end of life. The [VHDL][] is written in a generic way, with hardware components 
being inferred instead of explicitly instantiated, this should make the code 
fairly portable, although the interfaces to the [Nexys3][] board components are
specific to the peripherals on that board.

## License

The licenses used by the project are mixed and are on a per file basis. For my
code I use the [MIT][] license - so feel free to use it as you wish. The other
licenses used are the [LGPL][], they are confined to single modules so could be
removed if you have some aversion to [LGPL][] code.

## Target Board

The only target board available at the moment is the [Nexys3][], this should
change in the future as the board is currently at it's End Of Life. The next
boards I am looking to support are it's successor, the Nexys 4, and the myStorm
BlackIce (<https://mystorm.uk/>). The myStorm board uses a completely open
source toolchain for synthesis, place and route and bit file generation.

## Build requirements

The build has been tested under [Debian][] [Linux][], version 8.

You will require:

* [GCC][], or a suitable [C][] compiler capable of compiling [C99][]
* [Make][]
* [Xilinx ISE][] version 14.7
* [GHDL][]
* [GTKWave][]
* [tcl][] version 8.6
* Digilent Adept2 runtime and Digilent Adept2 utilities available at
  <http://store.digilentinc.com/digilent-adept-2-download-only/>

[Xilinx ISE][] can (or could be) downloaded for free, but requires
registration. ISE needs to be on your path:

	PATH=$PATH:/opt/Xilinx/14.7/ISE_DS/ISE/bin/lin64;
	PATH=$PATH:/opt/Xilinx/14.7/ISE_DS/ISE/lib/lin64;

## Building

To make a bit file that can be flashed to the target board:

	make simulation synthesis implementation bitfile

To upload the bitfile to the target board:

	make upload

To make the [C][] based toolchain:

	make h2

To view the wave form generated by "make simulation":

	make viewer

## Manual

The H2 processor and associated peripherals are subject to change, so the code
is the definitive source what instructions are available, the register map, and
how the peripherals behave.

There are a few modifications to the [J1][] CPU which include:

* New instructions
* A CPU hold line which keeps the processor in the same state so long as it is
high.
* Interrupt Service Routines have been added.

The Interrupt Service Routines (ISR) have not been throughly tested and will be
subject to the most change.

### H2 CPU

The H2 CPU behaves very similarly to the [J1][] CPU, and the [J1 PDF][] can be
read in order to better understand this processor. The processor is 16-bit with
instructions taking a single clock cycle. 

The CPU has the following state within it:

* A 32 deep return stack
* A 33 deep variable stack
* A program counter
* An interrupt enable and interrupt request bit
* An interrupt address register

Loads and stores into the block RAM that holds the H2 program discard the
lowest bit, every other memory operation uses the lower bit (such as jumps
and loads and stores to Input/Output peripherals). This is so applications can
use the lowest bit for character operations.

The instruction set is decoded in the following manner:

	*---------------------------------------------------------------*
	| F | E | D | C | B | A | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
	*---------------------------------------------------------------*
	| 1 |                    LITERAL VALUE                          |
	*---------------------------------------------------------------*
	| 0 | 0 | 0 |            BRANCH TARGET ADDRESS                  |
	*---------------------------------------------------------------*
	| 0 | 0 | 1 |            CONDITIONAL BRANCH TARGET ADDRESS      |
	*---------------------------------------------------------------*
	| 0 | 1 | 0 |            CALL TARGET ADDRESS                    |
	*---------------------------------------------------------------*
	| 0 | 1 | 1 |   ALU OPERATION   |T2N|T2R|N2A|R2P| RSTACK| DSTACK|
	*---------------------------------------------------------------*
	| F | E | D | C | B | A | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
	*---------------------------------------------------------------*

	T   : Top of data stack
	N   : Next on data stack
	PC  : Program Counter

	LITERAL VALUES : push a value onto the data stack
	CONDITIONAL    : BRANCHS pop and test the T
	CALLS          : PC+1 onto the return stack

	T2N : Move T to N
	T2R : Move T to top of return stack
	N2A : STORE T to memory location addressed by N
	R2P : Move top of return stack to PC

	RSTACK and DSTACK are signed values (twos compliment) that are
	the stack delta (the amount to increment or decrement the stack
	by for their respective stacks: return and data)

#### ALU OPERATIONS

All ALU operations replace T:

	*-------*----------------*-----------------------*
	| Value |   Operation    |     Description       |
	*-------*----------------*-----------------------*
	|   0   |       T        |  Top of Stack         |
	|   1   |       N        |  Copy T to N          |
	|   2   |     T + N      |  Addition             |
	|   3   |     T & N      |  Bitwise AND          |
	|   4   |     T | N      |  Bitwise OR           |
	|   5   |     T ^ N      |  Bitwise XOR          |
	|   6   |      ~T        |  Bitwise Inversion    |
	|   7   |     T = N      |  Equality test        |
	|   8   |     N < T      |  Signed comparison    |
	|   9   |     N >> T     |  Logical Right Shift  |
	|  10   |     T - 1      |  Decrement            |
	|  11   |       R        |  Top of return stack  |
	|  12   |      [T]       |  Load from address    |
	|  13   |     N << T     |  Logical Left Shift   |
	|  14   |     depth      |  Depth of stack       |
	|  15   |     N u< T     |  Unsigned comparison  |
	|  16   | set interrupts |  Enable interrupts    |
	|  17   | interrupts on? |  Are interrupts on?   |
	|  18   |     rdepth     |  Depth of return stk  |
	|  19   |      0=        |  T == 0?              |
	*-------*----------------*-----------------------*

### Peripherals and registers

Registers marked prefixed with an 'o' are output registers, those with an 'i'
prefix are input registers. Registers are divided into an input and output
section of registers and the addresses of the input and output registers do not
correspond to each other in all cases. Unlike for RAM reads, the I/O registers
are indexed by word aligned addresses, without the lowest bit being discarded
(this should be fixed at a later date).

The following peripherals have been implemented in the [VHDL][] SoC to
interface with devices on the [Nexys3][] board:

* [VGA][] output device, text mode only, 80 by 40 characters from
  <http://www.javiervalcarce.eu/html/vhdl-vga80x40-en.html>
* Timer 
* [UART][] (Rx/Tx) with a [FIFO][]
from <https://github.com/pabennett/uart>
* [PS/2][] Keyboard
from <https://eewiki.net/pages/viewpage.action?pageId=28279002>
* [LED][] next to a bank of switches
* An [8 Segment LED Display][] driver (a 7 segment display with a decimal point)

The SoC also features a limited set of interrupts that can be enabled or
disabled.

	*---------------------------------------------------------*
	|                   Input Registers                       |
	*-------------*---------*---------------------------------*
	| Register    | Address | Description                     |
	*-------------*---------*---------------------------------*
	| iUart       | 0x6000  | UART register                   |
	| iSwitches   | 0x6001  | Buttons and switches            |
	| iTimerCtrl  | 0x6002  | Timer control Register          |
	| iTimerDin   | 0x6003  | Current Timer Value             |
	| iVgaTxtDout | 0x6004  | Contents of address oVgaTxtAddr |
	| iPs2        | 0x6005  | PS2 Keyboard Register           |
	*-------------*---------*---------------------------------*

	*---------------------------------------------------------*
	|                   Output Registers                      |
	*-------------*---------*---------------------------------*
	| Register    | Address | Description                     |
	*-------------*---------*---------------------------------*
	| oUart       | 0x6000  | UART register                   |
	| oLeds       | 0x6001  | LED outputs                     |
	| oTimerCtrl  | 0x6002  | Timer control                   |
	| oVgaCursor  | 0x6003  | VGA Cursor X/Y cursor position  |
	| oVgaCtrl    | 0x6004  | VGA control registers           |
	| o8SegLED    | 0x6005  | 4 x LED 8 Segment display 0     |
	| oIrcMask    | 0x6006  | CPU Interrupt Mask              |
	| VGA Memory  | 0xE000  | VGA memory                      |
	|             |    -    |                                 |
	|             | 0xFFFF  |                                 |
	*-------------*---------*---------------------------------*

The following description of the registers should be read in order and describe
how the peripherals work as well.

#### oUart

A UART with a fixed baud rate and format (115200, 8 bits, 1 stop bit) is
present on the SoC. The UART has a FIFO of depth 8 on both the RX and TX
channels. The control of the UART is split across oUart and iUart. 

To write a value to the UART assert TXWE along with putting the data in TXDO.
The FIFO state can be analyzed by looking at the iUart register.

To read a value from the UART: iUart can be checked to see if data is present
in the FIFO, if it is assert RXRE in the oUart register, on the next clock
cycle the data will be present in the iUart register.

The baud rate of the UART can be changed by rebuilding the VHDL project, bit
length, parity bits and stop bits can only be changed with modifications to
[uart.vhd][]

	*-------------------------------------------------------------------------------*
	| 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
	*-------------------------------------------------------------------------------*
	|  X |  X |TXWE|  X |  X |RXRE|  X |  X |               TXDO                    |
	*-------------------------------------------------------------------------------*

	TXWE: UART RT Write Enable
	RXRE: UART RX Read Enable
	TXDO: Uart TX Data Output


#### oLeds

On the [Nexys3][] board there is a bank of LEDs that are situated next to the
switches, these LEDs can be turned on (1) or off (0) by writing to LEDO. Each
LED here corresponds to the switch it is next to.

	*-------------------------------------------------------------------------------*
	| 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
	*-------------------------------------------------------------------------------*
	|  X |  X |  X |  X |  X |  X |  X |  X |              LEDO                     |
	*-------------------------------------------------------------------------------*

	LEDO: LED Output

#### oTimerCtrl

The timer is controllable by the oTimerCtrl register, it is a 13-bit timer
running at 100MHz, it can optionally generate interrupts and the current timers
internal count can be read back in with the iTimerDin register.

The timer counts once the TE bit is asserted, once the timer reaches TCMP value
it wraps around and can optionally generate an interrupt by asserting INTE.
This also toggles the Q and NQ lines that come out of the timer and are routed
to pins on the board (see the constraints file [top.ucf][] for the pins).

The timer can be reset by writing to RST.

	*-------------------------------------------------------------------------------*
	| 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
	*-------------------------------------------------------------------------------*
	| TE | RST|INTE|                      TCMP                                      |
	*-------------------------------------------------------------------------------*

	TE:   Timer Enable
	RST:  Timer Reset
	INTE: Interrupt Enable
	TCMP: Timer Compare Value

#### oVgaCursor

The VGA Text peripheral has a cursor, the cursor position can be changed with
this register.

	*-------------------------------------------------------------------------------*
	| 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
	*-------------------------------------------------------------------------------*
	|  X |  X |          POSY               |  X |            POSX                  |
	*-------------------------------------------------------------------------------*

	POSY: VGA Text Cursor Position Y
	POSX: VGA Text Cursor Position X


#### oVgaCtrl

The VGA control register contains bits that affect the behavior of the VGA Text
display. The VGA peripheral is a text only display, each location in video ram
gets written out to the display as a character. The display is monochrome, but
which color is used can be selected with the RED (for red), GRN (for green) and
BLU (for blue) bits in the oVgaCtrl register.

The CEN bit enables the cursor, and the BLK bit makes the cursor blink. The MOD
bit changes the cursors shape.

	*-------------------------------------------------------------------------------*
	| 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
	*-------------------------------------------------------------------------------*
	|  X |  X |  X |  X |  X |  X |  X |  X |  X | VEN| CEN| BLK| MOD| RED| GRN| BLU|
	*-------------------------------------------------------------------------------*

	VEN: VGA Enable
	CEN: Cursor Enable
	BLK: Cursor Blink
	MOD: Cursor Mode
	RED: Red Enable
	GRN: Green Enable
	BLU: Blue Enable

#### o8SegLED

On the [Nexys3][] board there is a bank of 7 segment displays, with a dot
(8-segment really), which can be used for numeric output. The LED segments
cannot be directly addressed. Instead the value stored in L8SD is mapped
to a hexadecimal display value (or a BCD value, but this requires regeneration 
of the SoC and modification of a generic in the VHDL).

The value '0' corresponds to a zero displayed on the LED segment, '15' to an
'F', etcetera.

There are 4 displays in a row.

	*-------------------------------------------------------------------------------*
	| 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
	*-------------------------------------------------------------------------------*
	|      L8SD0        |       L8SD1       |       L8SD2       |       L8SD3       |
	*-------------------------------------------------------------------------------*

	L8SD0: LED 8 Segment Display (leftmost display)
	L8SD1: LED 8 Segment Display
	L8SD2: LED 8 Segment Display
	L8SD3: LED 8 Segment Display (right most display)

#### oIrcMask

The H2 core has a mechanism for interrupts, interrupts have to be enabled or
disabled with an instruction. Each interrupt can be masked off with a bit in
IMSK to enable that specific interrupt. A '1' in a bit of IMSK enables that
specific interrupt, which will be delivered to the CPU if interrupts are
enabled within it.

	*-------------------------------------------------------------------------------*
	| 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
	*-------------------------------------------------------------------------------*
	|  X |  X |  X |  X |  X |  X |  X |  X |                 IMSK                  |
	*-------------------------------------------------------------------------------*

	IMSK: Interrupt Mask

#### VGA Memory

The VGA memory occupies the range 0xE000 to 0xFFFF, it can be written to (but
not read from) like normal memory, except like all I/O registers the lowest bit
is used for addressing, whereas in normal memory it is not. The lowest byte is
display on the screen out of the 16-bit value. 

The value stored is treated as a [ISO 8859-1 (Latin-1)][] character (which is
an extended [ASCII][] character set.

#### iUart

The iUart register works in conjunction with the oUart register. The status of
the FIFO that buffers both transmission and reception of bytes is available in
the iUart register, as well as any received bytes.

	*-------------------------------------------------------------------------------*
	| 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
	*-------------------------------------------------------------------------------*
	|  X |  X |  X |TFFL|TFEM|  X |RFFL|RFEM|                RXDI                   |
	*-------------------------------------------------------------------------------*

	TFFL: UART TX FIFO Full
	TFEM: UART TX FIFO Empty
	RFFL: UART RX FIFO Full
	RFEM: UART RX FIFO Empty
	RXDI: UART RX Data Input

#### iSwitches

iSwitches contains input lines from multiple sources. The RX bit corresponds to
the UART input line, it is the raw input without any processing. The buttons
(BUP, BDWN, BLFT, BRGH, and BCNT) correspond to a [D-Pad][] on the [Nexys3][]
board. The switches (TSWI) are the ones mentioned in oLeds, each have an LED
next to them. 

The switches and the buttons are already debounced in hardware so they do not
have to be further processed once read in from these registers.

	*-------------------------------------------------------------------------------*
	| 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
	*-------------------------------------------------------------------------------*
	|  X |  X | RX | BUP|BDWN|BLFT|BRGH|BCNT|               TSWI                    |
	*-------------------------------------------------------------------------------*

	RX:   UART RX Line
	BUP:  Button Up
	BDWN: Button Down
	BLFT: Button Left
	BRGH: Button Right
	BCNT: Button Center
	TSWI: Two Position Switches

#### iTimerCtrl

This register contains the contents stored in oTimerCtrl.

	*-------------------------------------------------------------------------------*
	| 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
	*-------------------------------------------------------------------------------*
	| TE | RST|INTE|                      TCMP                                      |
	*-------------------------------------------------------------------------------*

	TE:   Timer Enable
	RST:  Timer Reset
	INTE: Interrupt Enable
	TCMP: Timer Compare Value

#### iTimerDin

This register contains the current value of the timers counter. 

	*-------------------------------------------------------------------------------*
	| 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
	*-------------------------------------------------------------------------------*
	|  X |  X |  X |                       TCNT                                     |
	*-------------------------------------------------------------------------------*

	TCNT: Timer Counter Value

#### iVgaTxtDout

This register contains the value of the video memory index by oVgaTxtAddr. The
mechanism for reading from VGA ram does not work correctly at the moment.

	*-------------------------------------------------------------------------------*
	| 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
	*-------------------------------------------------------------------------------*
	|                                     VRDO                                      |
	*-------------------------------------------------------------------------------*

	VRDO: VGA RAM Data Output

#### iPs2

This register contains the interface to the PS/2 keyboard. If PS2N is set then
an [ASCII][] character is present in ACHR. Both PS2N and ACHR will be cleared.

	*-------------------------------------------------------------------------------*
	| 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
	*-------------------------------------------------------------------------------*
	|  X |  X |  X |  X |  X |  X |  X |PS2N|  X |              ACHR                |
	*-------------------------------------------------------------------------------*

	PS2N: New character available on PS2 Keyboard
	ACHR: ASCII Character

#### Interrupt Service Routines

The following interrupt service routines are defined:

	*-------------------*--------*-----------------------------*
	|       Name        | Number |         Description         |
	*-------------------*--------*-----------------------------*
	| isrNone           |   0    | Not used                    |
	| isrRxFifoNotEmpty |   1    | UART RX FIFO Is Not Empty   |
	| isrRxFifoFull     |   2    | UART RX FIFI Is Full        |
	| isrTxFifoNotEmpty |   3    | UART TX FIFO Is Not Empty   |
	| isrTxFifoFull     |   4    | UART TX FIFO Is Full        |
	| isrKbdNew         |   5    | New PS/2 Keyboard Character |
	| isrTimer          |   6    | Timer Counter               |
	| isrBrnLeft        |   7    | Left D-Pad button pressed   |
	*-------------------*--------*-----------------------------*

When an interrupt occurs, and interrupts are enabled within the processor, then
a call to the location in memory is performed - the location is the same as the
ISR number. An ISR with a number of '4' will perform a call (not a jump) to the
location '4' within memory, for example.

Interrupts have a latency of at least 4-5 cycles before they are acted on, there
is a two to three cycle delay in the interrupt request handler, then the call
to the ISR location in memory has to be done, then the call to the word that
implements the ISR itself.

If two interrupts occur at the same time they are processed from the lowest
interrupt number to the highest.

Interrupts are lost when an interrupt with the same number occurs that has not
been processed.

### Assembler, Disassembler and Simulator

The Assembler, Disassembler and [C][] based simulator for the H2 is in a single
program (see [h2.c][]). This simulator complements the [VHDL][] test bench
[tb.vhd][] and is not a replacement for it.

#### Assembler

The assembler is actually a compiler for a pseudo Forth like language with a
fixed grammar. It is a much more restricted language than Forth and cannot be
extended within itself like Forth can. 

The main program can be found in [h2.fth][], which is still currently in
testing. 

The assembler/compiler reads in a text file containing a program and produces a
hex file which can be read in by the simulator, disassembler, the VHDL test
bench or read in by the [Xilinx ISE][] toolchain when it generates the bit file
for the [Spartan 6][] on the [Nexys3][] board.

#### Disassembler

The disassembler takes a text file containing the assembled program, which
consists of 16-bit hexadecimal numbers. It then attempts to disassemble the
instructions. It can also be fed a symbols file which can be generated by the
assembler and attempt to find the locations jumps and calls point to.

#### Simulator

The simulator in C implements the H2 core and most of the SoC. The IO for the
simulator is not cycle accurate (and most likely will never be), but can be
used for running programs.

The simulator will eventually include a debugger, similar in nature to
[DEBUG.COM][] available in [DOS][].

### Coding standards

#### VHDL

#### C

#### FORTH

## To Do

* Make a bootloader/program loader
* Implement Forth interpreter on device
* Memory interface to Nexys 3 board on board memory
* A [Wishbone interface][] could be implemented for the H2 core
and peripherals
* Make a utility for generating text for the VGA screen.
* Investigate:
	- The H2 simulator needs all of its peripherals checking,
	specifically the VGA memory.
	- The TX FIFO Full signal is apparently not working.

## Forth

* The on board memory could be linked up to the Forth block
word set.
* Most of the Forth code could be taken from my [libforth][]
project.

## Resources

* <https://nanode0000.wordpress.com/2017/04/08/exploring-the-j1-instruction-set-and-architecture/>
* <https://www.fpgarelated.com/showarticle/790.php>
* <https://opencores.org/>
* <https://en.wikipedia.org/wiki/Peephole_optimization>
* <https://en.wikipedia.org/wiki/Superoptimization>
* <https://github.com/samawati/j1eforth>
* <https://github.com/jamesbowman/j1>

[DEBUG.COM]: https://en.wikipedia.org/wiki/Debug_%28command%29
[DOS]: https://en.wikipedia.org/wiki/DOS
[h2.c]: h2.c
[h2.fth]: h2.fth
[tb.vhd]: tb.vhd
[uart.vhd]: uart.vhd
[top.ucf]: top.ucf
[font.bin]: font.bin
[text.bin]: text.bin
[J1]: http://www.excamera.com/sphinx/fpga-j1.html
[J1 PDF]: http://excamera.com/files/j1.pdf
[PL/0]: https://github.com/howerj/pl0
[libforth]: https://github.com/howerj/libforth/
[MIT]: https://en.wikipedia.org/wiki/MIT_License
[LGPL]: https://www.gnu.org/licenses/lgpl-3.0.en.html
[VHDL]: https://en.wikipedia.org/wiki/VHDL
[Verilog]: https://en.wikipedia.org/wiki/Verilog
[UART]: https://en.wikipedia.org/wiki/Universal_asynchronous_receiver/transmitter
[FORTH]: https://en.wikipedia.org/wiki/Forth_%28programming_language%29
[VGA]: https://en.wikipedia.org/wiki/VGA
[Nexys3]: http://store.digilentinc.com/nexys-3-spartan-6-fpga-trainer-board-limited-time-see-nexys4-ddr/
[Make]: https://en.wikipedia.org/wiki/Make_%28software%29
[C]: https://en.wikipedia.org/wiki/C_%28programming_language%29
[Debian]: https://en.wikipedia.org/wiki/Debian
[Linux]: https://en.wikipedia.org/wiki/Linux
[GCC]: https://en.wikipedia.org/wiki/GNU_Compiler_Collection
[Xilinx ISE]: https://www.xilinx.com/products/design-tools/ise-design-suite.html
[Xilinx]: https://www.xilinx.com
[GHDL]: http://ghdl.free.fr/
[GTKWave]: http://gtkwave.sourceforge.net/
[C99]: https://en.wikipedia.org/wiki/C99
[tcl]: https://en.wikipedia.org/wiki/Tcl
[Wishbone interface]: https://en.wikipedia.org/wiki/Wishbone_%28computer_bus%29
[D-Pad]: https://en.wikipedia.org/wiki/D-pad
[FIFO]: https://en.wikipedia.org/wiki/FIFO_%28computing_and_electronics%29
[UART]: https://en.wikipedia.org/wiki/Universal_asynchronous_receiver/transmitter
[VGA]: https://en.wikipedia.org/wiki/Video_Graphics_Array
[PS/2]: https://en.wikipedia.org/wiki/PS/2_port
[LED]: https://en.wikipedia.org/wiki/Light-emitting_diode
[8 Segment LED Display]: https://en.wikipedia.org/wiki/Seven-segment_display
[ISO 8859-1 (Latin-1)]: https://cs.stanford.edu/people/miles/iso8859.html
[Spartan 6]: https://www.xilinx.com/products/silicon-devices/fpga/spartan-6.html
[FPGA]: https://en.wikipedia.org/wiki/Field-programmable_gate_array
[ASCII]: https://en.wikipedia.org/wiki/ASCII

<style type="text/css">body{margin:40px auto;max-width:850px;line-height:1.6;font-size:16px;color:#444;padding:0 10px}h1,h2,h3{line-height:1.2}</style>
