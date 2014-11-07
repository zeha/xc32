#objdump: -dr --prefix-addresses --show-raw-insn
#name: MIPS branch-misc-2
#as: -32

# Test the branches to local symbols in current file.

.*: +file format .*mips.*

Disassembly of section .text:
	\.\.\.
	\.\.\.
	\.\.\.
0+003c <[^>]*> 0411fff0 	bal	00000000 <g1>
0+0040 <[^>]*> 00000000 	nop
0+0044 <[^>]*> 0411fff3 	bal	00000014 <g2>
0+0048 <[^>]*> 00000000 	nop
0+004c <[^>]*> 0411fff6 	bal	00000028 <g3>
0+0050 <[^>]*> 00000000 	nop
0+0054 <[^>]*> 0411000a 	bal	00000080 <g4>
0+0058 <[^>]*> 00000000 	nop
0+005c <[^>]*> 0411000d 	bal	00000094 <g5>
0+0060 <[^>]*> 00000000 	nop
0+0064 <[^>]*> 04110010 	bal	000000a8 <g6>
0+0068 <[^>]*> 00000000 	nop
	\.\.\.
	\.\.\.
	\.\.\.
0+00a8 <g6> 1000ffff 	b	000000a8 <g6>
			a8: R_MIPS_PC16	x1
0+00ac <g6+0x4> 00000000 	nop
0+00b0 <g6+0x8> 1000ffff 	b	000000b0 <g6+0x8>
			b0: R_MIPS_PC16	x2
0+00b4 <g6+0xc> 00000000 	nop
0+00b8 <g6+0x10> 1000ffff 	b	000000b8 <g6+0x10>
			b8: R_MIPS_PC16	.data
0+00bc <g6+0x14> 00000000 	nop

	\.\.\.
