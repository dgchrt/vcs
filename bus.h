#ifndef BUS_H
#define BUS_H

#define BUS_MEMORY_SIZE 0x2000
#define BUS_MEMORY_MASK (BUS_MEMORY_SIZE - 1)

extern unsigned char bus_memory[BUS_MEMORY_SIZE];

unsigned char bus_read(unsigned short address);
void bus_write(unsigned short address, unsigned char value);

#endif
