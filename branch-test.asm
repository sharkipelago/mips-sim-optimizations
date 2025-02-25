
branch-test:     file format elf32-tradlittlemips


Disassembly of section .text:

00000000 <__start>:
   0:	20210001 	addi	at,at,1
   4:	20a50005 	addi	a1,a1,5
   8:	10a50008 	beq	a1,a1,2c <skip>
   c:	00000000 	nop
  10:	20210001 	addi	at,at,1
  14:	20210001 	addi	at,at,1
  18:	20210001 	addi	at,at,1
  1c:	20210001 	addi	at,at,1
  20:	20210001 	addi	at,at,1
  24:	20210001 	addi	at,at,1
  28:	20210001 	addi	at,at,1

0000002c <skip>:
  2c:	20420002 	addi	v0,v0,2
  30:	20420002 	addi	v0,v0,2
  34:	20420002 	addi	v0,v0,2
  38:	14450004 	bne	v0,a1,4c <endlabel>
  3c:	00000000 	nop
  40:	20420002 	addi	v0,v0,2
  44:	20420002 	addi	v0,v0,2
  48:	20420002 	addi	v0,v0,2

0000004c <endlabel>:
  4c:	00000000 	nop
