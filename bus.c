#include "bus.h"
#include "tia.h"

unsigned char bus_memory[BUS_MEMORY_SIZE];

unsigned char bus_read(unsigned short address) {
    if (address < ADDRESS_END_TIA) {
        return tia_read(&tia, address & 0x7F);
    }
    return bus_memory[address & BUS_MEMORY_MASK];
}

void bus_write(unsigned short address, unsigned char value) {
    if (address < ADDRESS_END_TIA) {
        tia_write(&tia, address & 0x7F, value);
    } else if (address < ADDRESS_END_RIOT_RAM) {
        bus_memory[address & BUS_MEMORY_MASK] = value;
    } else {
        bus_memory[address & BUS_MEMORY_MASK] = value;
    }
}
