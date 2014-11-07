#objdump: -dr -mmips:isa32r2
#as: -march=24kfx -mfix-24k-e48
#name: 24K E48: delay slot filling

.*: +file format .*mips.*

Disassembly of section .text:
00000000 <func>:
   0:	24620005 	addiu	v0,v1,5
   4:	8c440000 	lw	a0,0\(v0\)
   8:	ac430004 	sw	v1,4\(v0\)
   c:	10600002 	beqz	v1,18 <func\+0x18>
  10:	00000000 	nop
  14:	8c430008 	lw	v1,8\(v0\)
  18:	8c450010 	lw	a1,16\(v0\)
  1c:	00000000 	nop
