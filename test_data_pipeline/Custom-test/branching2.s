.set noat
.text
.align 2
.globl __start
.ent __start
.type __start, @function

__start:
  addi $1, $0, 3        # $1 = i (outer loop counter)
  addi $10, $0, 0       # $10 = result accumulator
  addi $20, $0, 100     # $20 = base address for memory store

loop_i_check:
  slti $2, $1, 1
  bne $2, $0, done      # if i < 1, go to done

  addi $3, $0, 2        # $3 = j (inner loop counter)

loop_j_check:
  slti $4, $3, 1
  bne $4, $0, dec_i     # if j < 1, go to dec_i

  add $5, $1, $3        # $5 = i + j
  andi $6, $5, 1        # check even/odd
  beq $6, $0, even_case # if even, go to even_case

# odd_case:
  addi $7, $5, 1        # $7 = (i + j) + 1
  beq $0, $0, mul_by_2  # unconditional branch

even_case:
  addi $7, $5, -1       # $7 = (i + j) - 1

# mul_by_2:
mul_by_2:
  sll $9, $7, 1         # $9 = $7 * 2

  add $10, $10, $9      # accumulate result
  sw $9, 0($20)         # store intermediate
  addi $20, $20, 4      # move to next word

  addi $3, $3, -1       # j--
  beq $0, $0, loop_j_check  # back to inner loop condition

dec_i:
  addi $1, $1, -1       # i--
  beq $0, $0, loop_i_check  # back to outer loop

done:
  sw $10, 0($0)         # final result in memory[0]

.end __start
.size __start, .-__start
