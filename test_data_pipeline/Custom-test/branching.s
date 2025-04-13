.set noat
.text
.align  2
.globl  __start
.ent    __start
.type   __start, @function

__start:
  addi $1, $0, 5        # $1 = outer loop counter (i)
  addi $2, $0, 0        # $2 = result accumulator
  addi $3, $0, 0        # $3 = even count
  addi $4, $0, 0        # $4 = odd count

outer_loop:
  slti $5, $1, 1        # if $1 < 1
  bne  $5, $0, done     # exit if i < 1

  andi $6, $1, 1        # check if i is even or odd
  beq  $6, $0, is_even  # branch to is_even if i % 2 == 0

is_odd:
  addi $4, $4, 1        # increment odd counter
  addi $7, $0, 3        # $7 = inner loop counter (3 times)
  j inner_loop

is_even:
  addi $3, $3, 1        # increment even counter
  addi $7, $0, 2        # $7 = inner loop counter (2 times)

inner_loop:
  slti $8, $7, 1
  bne  $8, $0, update   # exit inner loop if $7 < 1

  add  $2, $2, $1       # accumulate current i
  addi $7, $7, -1       # decrement inner loop counter
  j inner_loop

update:
  addi $1, $1, -1       # decrement outer loop counter
  j outer_loop

done:
  sw $2, 0($0)          # store result
  sw $3, 4($0)          # store even count
  sw $4, 8($0)          # store odd count

.end    __start
.size   __start, .-__start
