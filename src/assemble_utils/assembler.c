/**
 * @file assembler.c
 * @brief File for functions used to assemble.
 */

#include "assembler.h"

static word_t assemble_dpi(string_array_t *tokens);
static word_t assemble_spl(string_array_t *tokens);
static word_t assemble_mul(string_array_t *tokens);
static word_t assemble_sdt(string_array_t *tokens, word_array_t *extra_words,
                    int current_line, int max_lines);
static word_t assemble_bra(string_array_t *tokens, symbol_table_t *symbol_table,
                    address_t current_line);

/** An empty instruction constant. */
static const instruction_t NULL_INSTRUCTION = {
  .type = NUL,
  .immediate_value = 0,
  .rn = -1,
  .rd = -1,
  .rs = -1,
  .rm = -1,
  .flag_0 = false,
  .flag_1 = false,
  .flag_2 = false,
  .flag_3 = false,
  .shift_amount = 0,
};

/**
 * @brief Assembles a data processing instruction.
 *
 * @param tokens The string to parse.
 * @returns A machine code instruction.
 */
static word_t assemble_dpi(string_array_t *tokens) {
  instruction_t instruction = NULL_INSTRUCTION;
  char **sections = tokens->array;

  instruction.type = DPI;
  instruction.cond = AL;
  instruction.operation = mnemonic_to_opcode(string_to_mnemonic(sections[0]));

  string_array_t *operand_tokens = malloc(sizeof(string_array_t));
  if (!operand_tokens) {
    perror("Unable to allocate memory for operand_tokens.\n");
    exit(EXIT_FAILURE);
  }

  switch (instruction.operation) {
    case AND:
    case EOR:
    case SUB:
    case RSB:
    case ADD:
    case ORR:
      instruction.rd = string_to_reg_address(sections[1]);
      instruction.rn = string_to_reg_address(sections[2]);

      operand_tokens->array = &sections[3];
      operand_tokens->size = tokens->size - 3;
      parse_operand(operand_tokens, &instruction);
      break;
    case MOV:
      instruction.rd = string_to_reg_address(sections[1]);

      operand_tokens->array = &sections[2];
      operand_tokens->size = tokens->size - 2;
      parse_operand(operand_tokens, &instruction);
      break;
    case TST:
    case TEQ:
    case CMP:
      instruction.rn = string_to_reg_address(sections[1]);
      instruction.flag_1 = true;
      operand_tokens->array = &sections[2];
      operand_tokens->size = tokens->size - 2;
      parse_operand(operand_tokens, &instruction);
      break;
  }

  free(operand_tokens);
  return encode(&instruction);
}

/**
 * @brief Assembles a special instruction.
 *
 * @param tokens The string to parse.
 * @returns A machine code instruction.
 */
static word_t assemble_spl(string_array_t *tokens) {
  instruction_t instruction = NULL_INSTRUCTION;

  if (4 == tokens->size) {
    // Is an ANDEQ instruction
    instruction.type = ZER;
  } else if (3 == tokens->size) {
    // Build the tokens for assemble_dpi to make into a MOV instruction
    string_array_t *mov_tokens = malloc(sizeof(string_array_t));
    if (!mov_tokens) {
      perror("Unable to allocate memory for mov_tokens.\n");
      exit(EXIT_FAILURE);
    }

    mov_tokens->size = 5;
    mov_tokens->array = malloc(sizeof(char *) * 5);
    if (!mov_tokens->array) {
      perror("Unable to allocate memory for mov_tokens array.\n");
      exit(EXIT_FAILURE);
    }
    mov_tokens->array[0] = "mov";
    // Rn
    mov_tokens->array[1] = tokens->array[1];
    // Rn
    mov_tokens->array[2] = tokens->array[1];
    // lsl
    mov_tokens->array[3] = tokens->array[0];
    // <#expression>
    mov_tokens->array[4] = tokens->array[2];

    word_t mov_instruction = assemble_dpi(mov_tokens);
    free(mov_tokens->array);
    free(mov_tokens);

    return mov_instruction;
  } else {
    fprintf(stderr, "Incorrect number of operands for shift or andeq\n");
    exit(EXIT_FAILURE);
  }

  return encode(&instruction);
}

/**
 * @brief Assembles a multiply instruction.
 *
 * @param tokens The string to parse.
 * @returns A machine code instruction.
 */
static word_t assemble_mul(string_array_t *tokens) {
  instruction_t instruction = NULL_INSTRUCTION;

  instruction.type = MUL;
  instruction.cond = AL;
  instruction.rd = string_to_reg_address(tokens->array[1]);
  instruction.rm = string_to_reg_address(tokens->array[2]);
  instruction.rs = string_to_reg_address(tokens->array[3]);

  if (5 == tokens->size) {
    // Is an MLA instruction
    instruction.flag_0 = 1;
    instruction.rn = string_to_reg_address(tokens->array[4]);
  }

  return encode(&instruction);
}

/**
 * @brief Assembles a single data transfer instruction.
 *
 * @param tokens The string to parse.
 * @param extra_words An array to hold additional words required by ldr.
 * @param current_line The line number of this instruction.
 * @param max_lines The maximum number of instructions.
 * @returns A machine code instruction.
 */
static word_t assemble_sdt(string_array_t *tokens, word_array_t *extra_words,
                    int current_line, int max_lines) {
  instruction_t instruction = NULL_INSTRUCTION;
  char **sections = tokens->array;

  instruction.type = SDT;
  instruction.cond = AL;
  instruction.rd = string_to_reg_address(sections[1]);
  instruction.flag_2 = 1;
  if (string_to_mnemonic(sections[0]) == LDR_M) {
    instruction.flag_3 = 1;
  }

  if ('=' == sections[2][0]) {
    // In form <=expression>

    // Get the value of the expression
    word_t expression_value = parse_immediate_value(&sections[2][1]);

    if (expression_value <= 0xFF) {
      // Build the tokens for assemble_dpi to make into a MOV instruction
      string_array_t *mov_tokens = malloc(sizeof(string_array_t));
      if (!mov_tokens) {
        perror("Unable to allocate memory for mov_tokens.\n");
        exit(EXIT_FAILURE);
      }

      mov_tokens->size = 3;
      mov_tokens->array = malloc(sizeof(char *) * 3);
      if (!mov_tokens->array) {
        perror("Unable to allocate memory for mov_tokens array.\n");
        exit(EXIT_FAILURE);
      }
      mov_tokens->array[0] = "mov";
      // Rn
      mov_tokens->array[1] = sections[1];
      // <#expression>
      mov_tokens->array[2] = sections[2];
      mov_tokens->array[2][0] = '#';

      word_t mov_instruction = assemble_dpi(mov_tokens);
      free(mov_tokens->array);
      free(mov_tokens);

      return mov_instruction;
    } else {
      // Store expression value at end of memory
      instruction.flag_1 = 1;
      instruction.rn = PC;
      instruction.immediate_value = (((max_lines + extra_words->size)
                                      - current_line) << 2) - 8;
      add_word_array(extra_words, expression_value);
    }
  } else {
    if (5 == tokens->size) {
      // [Rn]
      instruction.rn = string_to_reg_address(sections[3]);
      instruction.flag_1 = 1;
    } else {
      if (']' == sections[4][0]) {
        // Post indexing address specification
        if ('#' == sections[5][0]) {
          // [Rn], <#expression>
          instruction.rn = string_to_reg_address(sections[3]);
          instruction.immediate_value = parse_immediate_value(&sections[5][1]);
        } else {
          // [Rn],{+/-}Rm{,<shift>}
          instruction.rn = string_to_reg_address(sections[3]);
          instruction.flag_0 = 1;

          if ('-' == sections[5][0]) {
            instruction.flag_2 = 0;
            instruction.rm = string_to_reg_address(&sections[5][1]);
          } else {
            instruction.rm = string_to_reg_address(sections[5]);
          }

          if (tokens->size > 6) {
            // Has shift
            string_array_t *shift_tokens = malloc(sizeof(string_array_t));

            if (!shift_tokens) {
              perror("Unable to allocate memory for shift_tokens.\n");
              exit(EXIT_FAILURE);
            }

            // Pass the <shift> section into parse_shift
            shift_tokens->array = &sections[6];
            shift_tokens->size = tokens->size - 6;
            parse_shift(shift_tokens, &instruction);
            free(shift_tokens);
          }
        }
      } else {
        // Pre indexing address specification
        instruction.flag_1 = 1;
        instruction.rn = string_to_reg_address(sections[3]);

        if ('#' == sections[4][0]) {
          // [Rn, <#expression>]
          // Set flags if the immediate expression is negative
          word_t value = parse_immediate_value(&sections[4][1]);
          instruction.immediate_value = absolute(value);
          instruction.flag_2 = !is_negative(value);
        } else {
          // [Rn, {+/-}Rm{,<shift>}]
          instruction.flag_0 = 1;

          if ('-' == sections[4][0]) {
            instruction.flag_2 = 0;
            instruction.rm = string_to_reg_address(&sections[4][1]);
          } else {
            instruction.rm = string_to_reg_address(sections[4]);
          }

          if (tokens->size > 6) {
            // Has shift
            string_array_t *shift_tokens = malloc(sizeof(string_array_t));

            if (!shift_tokens) {
              perror("Unable to allocate memory for shift_tokens.\n");
              exit(EXIT_FAILURE);
            }

            // Pass the <shift> section into parse_shift
            shift_tokens->array = &sections[5];
            shift_tokens->size = tokens->size - 5;
            parse_shift(shift_tokens, &instruction);
            free(shift_tokens);
          }
        }
      }
    }
  }

  return encode(&instruction);
}

/**
 * @brief Assembles a branch instruction.
 *
 * @param tokens The string to parse.
 * @param symbol_table The table of labels and addresses.
 * @param instruction_no The line number of this instruction.
 * @returns A machine code instruction.
 */
static word_t assemble_bra(string_array_t *tokens, symbol_table_t *symbol_table,
                    address_t instruction_no) {
  instruction_t instruction = NULL_INSTRUCTION;

  instruction.type = BRA;

  if (!tokens->array[0][1]) {
    instruction.cond = AL;
  } else {
    instruction.cond = string_to_condition(&(tokens->array[0][1]));
  }

  address_t label_address = get_address(symbol_table, tokens->array[1]);

  instruction.immediate_value =
    (signed_to_twos_complement((int32_t) label_address
                               - ((int32_t) instruction_no * 4) - 8) >> 2);
  return encode(&instruction);
}

/**
 * @brief Assembles word arrays of tokenized instructions.
 *
 * @param instructions A pointer to the tokenized instructions.
 * @param symbol_table A pointer to the table of labels and addresses.
 * @param words A pointer to the array where assembled words are written.
 */
void assemble_all_instructions(string_arrays_t *instructions,
                               symbol_table_t *symbol_table,
                               word_array_t *words) {
  word_array_t *extra_words = make_word_array();
  int max_lines = instructions->size - symbol_table->size;
  word_t machine_instruction;
  for (int i = 0; i < instructions->size; i++) {
    if (instructions->arrays[i]->size != 1) {
      mnemonic_t ins_code = string_to_mnemonic(instructions->arrays[i]
                                               ->array[0]);
      switch(ins_code) {
        case ADD_M:
        case SUB_M:
        case RSB_M:
        case AND_M:
        case EOR_M:
        case ORR_M:
        case MOV_M:
        case TST_M:
        case TEQ_M:
        case CMP_M:
          machine_instruction = assemble_dpi(instructions->arrays[i]);
          break;
        case MUL_M:
        case MLA_M:
          machine_instruction = assemble_mul(instructions->arrays[i]);
          break;
        case LDR_M:
        case STR_M:
          machine_instruction = assemble_sdt(instructions->arrays[i],
                                             extra_words,
                                             words->size, max_lines);
          break;
        case BEQ_M:
        case BNE_M:
        case BGE_M:
        case BLT_M:
        case BGT_M:
        case BLE_M:
        case B_M:
          machine_instruction = assemble_bra(instructions->arrays[i],
                                             symbol_table, words->size);
          break;
        case SHIFT_M:
        case ANDEQ_M:
          machine_instruction = assemble_spl(instructions->arrays[i]);
          break;
        default:
          fprintf(stderr, "No such opcode found.\n");
          exit(EXIT_FAILURE);
          break;
      }
      add_word_array(words, machine_instruction);
    }
  }

  // Add extra words from LDR
  for (int i = 0; i < extra_words->size; i++) {
    add_word_array(words, extra_words->array[i]);
  }

  free_word_array(extra_words);
}
