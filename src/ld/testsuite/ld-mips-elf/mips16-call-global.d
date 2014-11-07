
.*:     file format elf.*mips

Disassembly of section .text:

.*0000 <__start>:
.*0000:	64c3      	save	24,ra
.*0002:	1800 8007 	jal	.*001c <y\+0x4>
.*0006:	6500      	nop
.*0008:	1c00 8008 	jalx	.*0020 <z>
.*000c:	6500      	nop
.*000e:	6443      	restore	24,ra
.*0010:	e8a0      	jrc	ra
.*0012:	6500      	nop

.*0014 <x>:
.*0014:	e8a0      	jrc	ra
.*0016:	6500      	nop

.*0018 <y>:
.*0018:	03e00008 	jr	ra
.*001c:	00000000 	nop

.*0020 <z>:
.*0020:	03e00008 	jr	ra
.*0024:	00000000 	nop
	\.\.\.
