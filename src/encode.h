#ifndef ENCODE_H
#define ENCODE_H
#include <stdio.h>
#include "instruction.h"
#include "global.h"

word_t add_cond(instruction_t *instruction);
word_t encode_dpi(instruction_t *instruction);
word_t encode_mul(instruction_t *instruction);
word_t encode_sdt(instruction_t *instruction);
word_t encode_branch(instruction_t *instruction);

#endif