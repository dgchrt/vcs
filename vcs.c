#include "vcs.h"
#include "bus.h"
#include "conio.h"
#include "mos6507.h"
#include <stdio.h>
#include <stdlib.h>

void vcs_cycle() {
  while (running) {
    mos6507_fetch();
    mos6507_decode();
    vcs_update();
  }
}

void vcs_load_rom(char path[]) {
  FILE *file;
  unsigned char byte;
  unsigned short i;

  file = fopen(path, "rb");

  if (!file) {
    printf("Can't open %s!\n", path);
    exit(1);
  }

  printf("Loading %s...\n", path);
  i = ROM_ADDRESS;

  while (fread(&byte, 1, 1, file) == 1) {
    bus_write(i++, byte);
  }

  fclose(file);
  program_counter =
      (bus_read(VECTOR_RESET) | (bus_read(VECTOR_RESET + 1) << BYTE_SIZE)) &
      BUS_MEMORY_MASK;
  // clrscr();
  vcs_cycle();
}

void vcs_update() {
  if (kbhit() && getch() == KEY_ESC) {
    running = 0;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s file.rom\n", argv[0]);
    exit(1);
  } else {
    vcs_load_rom(argv[1]);
    // clrscr();
  }

  return 0;
}
