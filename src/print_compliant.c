#include "print_compliant.h"

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
 * @brief Prints a value for test cases, hex and 2's complement.
 *
 * @param value The word to print.
 */
void print_value_compliant(word_t value) {
  printf("%10ld (0x%08x)\n", twos_complement_to_long(value), value);
}