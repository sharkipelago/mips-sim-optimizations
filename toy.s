  .set noat
    .text
    .align  2
    .globl  __start
    .ent    __start
    .type   __start, @function
__start:
   addi $2, $2, 1
   addi $3, $2, 7
   add $1, $2, $3
   addi $2, $1, 5
   j label
   sub $2, $2, $3
label:
   or $4, $3, $1
    .end    __start
    .size   __start, .-__start