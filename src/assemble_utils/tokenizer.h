/**
 * @file tokenizer.h
 * @brief Header file for tokenizer.c.
 */

#ifndef TOKENIZER_H
#define TOKENIZER_H
#include "assemble_toolbox.h"

bool is_label(char *instruction);
char *trim(char *string);
char *trim_token(char *string);
char *split_token(string_array_t *result, int *cur_section, char *operands, int i);

void free_tokens(string_arrays_t *tokenized_input);

string_arrays_t *tokenize_input(char **input, int input_lines);
string_array_t *tokenize_operand_instruction(string_array_t *result, char *instruction_op, char *operands);
string_array_t *tokenize_instruction(char *instruction);

#endif
