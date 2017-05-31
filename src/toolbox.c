#include "toolbox.h"

/**
 * @brief Loads a binary file into the memory.
 *
 * Writes the contents of the provided binary object code file to the memory,
 * starting at the provided location. Returns an error message and exits if the
 * file cannot be opened or cannot be read.
 * @param fname The filename conataining object code to be loaded.
 * @param memory A pointer to the first byte of memory to be written to.
 */
 void load_file(char *fname, byte_t *memory) {
   // Try to open the file
   FILE *file = fopen(fname, "rb");
   if (file == NULL) {
     perror("Error in opening object code file.");
     exit(EXIT_FAILURE);
   }
   // Try to read all lines into the memory
   fread(memory, NUM_ADDRESSES, 1, file);
   if(ferror(file)) {
     perror("Error in reading from object code file.");
     exit(EXIT_FAILURE);
   }
   // Close the file
   fclose(file);
}

/**
 * @brief Exits gracefully.
 *
 * Prints the current system state, frees allocated memory and exits with a
 * failure. To be used in the case of an error which cannot be recovered from.
 * @param machine The current system state.
 */
void exit_program(system_state_t *machine) {
  print_system_state(machine);
  free(machine->decoded_instruction);
  free(machine);
  exit(EXIT_FAILURE);
}

/**
 * @brief Gets a memory word from a given address.
 *
 * @param machine The current system state.
 * @param mem_address The memory address to be read from.
 * @returns The word at the given memory address in the current system state.
 */
word_t get_word(system_state_t *machine, uint32_t mem_address) {
  if (mem_address > NUM_ADDRESSES - 4) {
    if (COMPLIANT_MODE) {
      printf("Error: Out of bounds memory access at address 0x%08x\n",
             mem_address);
      return 0;
    } else {
      fprintf(stderr, "Address: %u was out of bounds in get_word", mem_address);
      exit_program(machine);
    }
  }
  word_t value = 0;
  for (size_t i = 0; i < 4; i++) {
    value |= ((word_t) machine->memory[mem_address + i]) << (i * 8);
  }
  return value;
}

/**
 * @brief Gets a memory word from a given address (for test cases).
 *
 * @param machine The current system state.
 * @param mem_address The memory address to be read from.
 * @returns The word at the given memory address in the current system state.
 */
word_t get_word_compliant(system_state_t *machine, address_t mem_address) {
  word_t value = 0;
  for (size_t i = 0; i < 4; i++) {
    value |= ((word_t) machine->memory[mem_address + 3 - i]) << (i * 8);
  }
  return value;
}

/**
 * @brief Writes a word to memory at a given address.
 *
 * @param machine The current system state.
 * @param mem_address The memory address to write to.
 * @param word The word to write to memory.
 */
void set_word(system_state_t *machine, uint32_t mem_address, word_t word) {
  if (mem_address > NUM_ADDRESSES - 4) {
    if (COMPLIANT_MODE) {
      printf("Error: Out of bounds memory access at address 0x%x\n",
             mem_address);
      return;
    } else {
      fprintf(stderr, "Address: %u was out of bounds in set_word", mem_address);
      exit_program(machine);
    }
  }
  for (size_t i = 0; i < 4; i++) {
    machine->memory[mem_address + i] = (byte_t) (word & 0xFF);
    word >>= 8;
  }
}

/**
 * @brief Negates a two's complement value.
 *
 * @param value The word to be negated.
 * @returns The negated word.
 */
word_t negate(word_t value) {
  return (~value) + 1;
}

/**
 * @brief Returns true iff two's complement value is negative.
 *
 * @param value A two's complement word to check sign of.
 * @returns True iff provided value is negative in two's complement.
 */
bool is_negative(word_t value) {
  return value >> 31;
}

/**
 * @brief Returns absolute two's complement value.
 *
 * @param value A two's complemnt word.
 * @returns The absolute value of the provided word in two's complement.
 */
word_t absolute(word_t value) {
  if (is_negative(value)) {
    return negate(value);
  }
  return value;
}

/**
 * @brief Prints a given number of bytes, from an array of bytes.
 *
 * Prints a given number bytes from memory, starting from address 0. Lines are
 * broken every word (4 bytes). Useful for debugging.
 * @param memory An array of bytes to print.
 * @param bytes_to_print The number of bytes to print (from 0).
 */
void print_array(byte_t *memory, size_t bytes_to_print) {
  for (size_t i = 0; i < bytes_to_print; i++) {
    printf("%x", memory[i]);
    // New line at each word
    if (i % 4 == 3) {
      printf("\n");
    }
  }
  printf("\n");
}

/**
 * @brief Prints system state details. (for test cases)
 *
 * Prints the current system state. Prints all register values, any memory
 * values which are not 0.
 */
void print_system_state_compliant(system_state_t *machine) {
  printf("Registers:\n");
  print_registers_compliant(machine);
  printf("Non-zero memory:\n");
  print_memory_compliant(machine);
}

/**
 * @brief Prints system state details.
 *
 * Prints the current system state. Prints all register values, any memory
 * values which are not 0, the decoded instruction and the fetched instruction.
 * @param machine The current system state.
 */
void print_system_state(system_state_t *machine) {
  printf("\n--------------------------------------------------\n\n");
  printf("System State:\n");
  print_registers(machine);
  print_memory(machine);
  print_decoded_instruction(machine);
  print_fetched_instruction(machine);
}

/**
 * @brief Prints the values of the registers of the machine.
 *
 * @param machine The arm machine that has the registers to print.
 */
void print_registers_compliant(system_state_t *machine) {
  for (uint8_t i = 0; i <= 12; ++i) {
    word_t value = machine->registers[i];
    printf("$%-2d : ", i);
    print_value_compliant(value);
  }

  printf("PC  : ");
  print_value_compliant(machine->registers[15]);

  printf("CPSR: ");
  print_value_compliant(machine->registers[16]);
}

/**
 * @brief Prints the values of registers.
 *
 * Prints the values held in each of the NUM_REGISTERS registers.
 * @param machine The current system state.
 */
void print_registers(system_state_t *machine) {
  printf("Register State:\n");
  for (uint8_t i = 0; i < NUM_REGISTERS; ++i) {
    word_t value = machine->registers[i];
    printf("Register %2d, ", i);
    print_value(value);
  }
}

/**
 * @brief Prints the memory of the machine that holds a value for test cases.
 *
 * @param machine The arm machine that has the memory to print.
 */
void print_memory_compliant(system_state_t *machine) {
  for (uint32_t i = 0; i < NUM_ADDRESSES; i += 4) {
    word_t value = get_word_compliant(machine, i);
    if (value) {
      printf("0x%08x: 0x%08x\n", i, value);
    }
  }
}

/**
 * @brief Prints any non-zero words from memory.
 *
 * Prints any non-zero words from memory and their addresses.
 * @param machine The current system state.
 */
void print_memory(system_state_t *machine) {
  printf("Memory State:\n");
  for (uint32_t i = 0; i < NUM_ADDRESSES; i += 4) {
    word_t value = get_word(machine, i);
    if (value) {
      printf("Memory Address %5d, ", i);
      print_value(value);
    }
  }
}


/**
 * @brief Prints details for the decoded instruction.
 *
 * Prints the type of the instruction, and any details required:
 * * For branch instructions, prints the condition and the offset.
 * *
 * @param machine The current system state.
 */
void print_decoded_instruction(system_state_t *machine) {
  switch (machine->decoded_instruction->type) {
    case NUL:
      printf("Decoded Instruction: None\n");
      break;
    case ZER:
      printf("Decoded Instruction: ZER\n");
      break;
    case BRA:
      printf("Decoded Instruction: BRA\n");
      printf("  Condition Flag: %s\n",
             get_cond(machine->decoded_instruction->cond));
      printf("  Offset: 0x%x\n",
             machine->decoded_instruction->immediate_value);
      break;
    case MUL:
      printf("Decoded Instruction: MUL\n");
      printf("  Condition Flag: %s\n",
             get_cond(machine->decoded_instruction->cond));
      if (machine->decoded_instruction->type == DPI) {
        printf("  Opcode: %s\n",
               get_opcode(machine->decoded_instruction->operation));
      }
      printf("  Immediate Value: 0x%x\n",
             machine->decoded_instruction->immediate_value);
      printf("  Destination Register: %d\n", machine->decoded_instruction->rd);
      break;
    case DPI:
      printf("Decoded Instruction: DPI\n");
      printf("  Condition Flag: %s\n",
             get_cond(machine->decoded_instruction->cond));
      if (machine->decoded_instruction->type == DPI) {
        printf("  Opcode: %s\n",
               get_opcode(machine->decoded_instruction->operation));
      }
      printf("  Immediate Value: 0x%x\n",
             machine->decoded_instruction->immediate_value);
      printf("  Destination Register: %d\n", machine->decoded_instruction->rd);
      break;
    case SDT:
      printf("Decoded Instruction: SDT\n");
      printf("  Condition Flag: %s\n",
             get_cond(machine->decoded_instruction->cond));
      if (machine->decoded_instruction->type == DPI) {
        printf("  Opcode: %s\n",
               get_opcode(machine->decoded_instruction->operation));
      }
      printf("  Immediate Value: 0x%x\n",
             machine->decoded_instruction->immediate_value);
      printf("  Destination Register: %d\n", machine->decoded_instruction->rd);
      break;
    default:
      assert(false);
  }
}

/**
 * @brief Prints the fetched instruction, if present.
 *
 * @param machine The current system state.
 */
void print_fetched_instruction(system_state_t *machine) {
  if (machine->has_fetched_instruction) {
    printf("Fetched Instruction, ");
    print_value(machine->fetched_instruction);
  } else {
    printf("Fetched Instruction: None\n");
  }
}


/**
 * @brief Prints a value for debugging, in binary, hex and 2's complement.
 *
 * @param value The word to print.
 */
void print_value(word_t value) {
  printf("Value: ");
  print_binary_value(value);
  printf(" (0x%08x) (%ld)\n", value, twos_complement_to_long(value));
}

/**
 * @brief Prints a value for test cases, hex and 2's complement.
 *
 * @param value The word to print.
 */
void print_value_compliant(word_t value) {
  printf("%10ld (0x%08x)\n", twos_complement_to_long(value), value);
}

/**
 * @brief Converts a signed 2's complement word to a sign long.
 *
 * @param value The signed 2's complement word to convert.
 * @returns The signed long representation of the word.
 */
long twos_complement_to_long(word_t value) {
  long result = absolute(value);
  if (is_negative(value)) {
    result *= -1;
  }
  return result;
}

/**
 * @brief Prints the padded binary representation of value.
 *
 * Prints WORD_SIZE bits.
 * @param value The word for printing.
 */
void print_binary_value(word_t value) {
  for (int i = 0; i < WORD_SIZE; ++i) {
    printf("%u", value >> (WORD_SIZE - 1));
    value <<= 1;
  }
}

/**
 * @brief Returns the string representing the condition type.
 *
 * @param type The condition type.
 * @returns The string of the condition type for printing.
 */
char *get_cond(condition_t cond) {
  switch (cond) {
    case eq:
      return "EQ";
    case ne:
      return "NE";
    case ge:
      return "GE";
    case lt:
      return "LT";
    case gt:
      return "GT";
    case le:
      return "LE";
    case al:
      return "AL";
    default:
      assert(false);
  }
}

/**
 * @brief Returns the string representing the instruction type.
 *
 * @param type The instruction type.
 * @returns The string of the instruction type for printing.
 */
char *get_type(instruction_type_t type) {
  switch (type) {
    case DPI:
      return "DPI";
    case MUL:
      return "MUL";
    case SDT:
      return "SDT";
    case BRA:
      return "BRA";
    case ZER:
      return "ZER";
    case NUL:
      return "NUL";
    default:
      assert(false);
  }
}

/**
 * @brief Returns the string representing the opcode.
 *
 * @param operation The opcode.
 * @returns The string of the opcode for printing.
 */
char *get_opcode(opcode_t operation) {
  switch (operation) {
    case AND:
      return "AND";
    case EOR:
      return "EOR";
    case SUB:
      return "SUB";
    case RSB:
      return "RSB";
    case ADD:
      return "ADD";
    case TST:
      return "TST";
    case TEQ:
      return "TEQ";
    case CMP:
      return "CMP";
    case ORR:
      return "ORR";
    case MOV:
      return "MOV";
    default:
      assert(false);
  }
}

/**
 * @brief Shifts a value and returns a pointer.
 *
 * @param type The type of shift to use.
 * @param shift_amount The amount to shift by.
 * @param value The value to shift.
 * @returns The pointer to the shifted value.
 */
 value_carry_t *shifter(shift_t type, word_t shift_amount, word_t value) {
   value_carry_t *result = malloc(sizeof(value_carry_t));

   if (!result) {
     perror("Unable to allocate memory for result of shifter");
     exit(EXIT_FAILURE);
   }

   switch(type) {
     case lsl:
       result->value = (shift_amount >= WORD_SIZE) ? 0 : value << shift_amount;
       result->carry = (value >> (WORD_SIZE - shift_amount)) & 0x1;
       break;
     case lsr:
       result->value = (shift_amount >= WORD_SIZE) ? 0 : value >> shift_amount;
       result->carry = (value << (WORD_SIZE - shift_amount)) & 0x80000000;
       break;
     case asr:
       result->value = (value >> shift_amount)
                       | ((value & 0x80000000) ?
                         ~((1L << (WORD_SIZE - shift_amount)) - 1L) : 0L);
       result->carry = (value << (WORD_SIZE - shift_amount)) & 0x80000000;
       break;
     case ror:
       result->value = (value << (WORD_SIZE - shift_amount))
                       | (value >> shift_amount);
       result->carry = (value << (WORD_SIZE - shift_amount)) & 0x80000000;
       break;
     default:
       fprintf(stderr, "Unknown shift type: %u", type);
       exit(EXIT_FAILURE);
   }

   if(shift_amount == 0) {
     result->carry = false;
   }

   return result;
 }
