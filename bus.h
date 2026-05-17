#include <stdint.h>

#ifndef BUS_H
#define BUS_H

#define BUS_MEMORY_SIZE 0x2000
#define BUS_MEMORY_MASK (BUS_MEMORY_SIZE - 1)

#define ADDRESS_END_TIA 0x80
#define ADDRESS_END_RIOT_RAM 0x100

extern uint8_t bus_memory[BUS_MEMORY_SIZE];

uint8_t bus_read(uint16_t address);
void bus_write(uint16_t address, uint8_t value);

#endif
