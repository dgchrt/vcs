#include "bus.h"

unsigned char bus_memory[BUS_MEMORY_SIZE];

unsigned char bus_read(unsigned short address) {
  return bus_memory[address & BUS_MEMORY_MASK];
}

void bus_write(unsigned short address, unsigned char value) {
  bus_memory[address & BUS_MEMORY_MASK] = value;
}
