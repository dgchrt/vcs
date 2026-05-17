#include "bus.h"
#include "tia.h"

uint8_t bus_memory[BUS_MEMORY_SIZE];

uint8_t bus_read(uint16_t address) {
  if (address < ADDRESS_END_TIA) {
    return tia_read(&tia, address & 0x7F);
  }
  return bus_memory[address & BUS_MEMORY_MASK];
}

void bus_write(uint16_t address, uint8_t value) {
  if (address < ADDRESS_END_TIA) {
    tia_write(&tia, address & 0x7F, value);
  } else if (address < ADDRESS_END_RIOT_RAM) {
    bus_memory[address & BUS_MEMORY_MASK] = value;
  } else {
    bus_memory[address & BUS_MEMORY_MASK] = value;
  }
}
