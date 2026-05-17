#include "vcs.h"
#include "bus.h"
#include "hal.h"
#include "mos6507.h"
#include "tia.h"
#include <stdio.h>
#include <stdlib.h>

void vcs_update() {
  if (kbhit() && getch() == KEY_ESC) {
    running = 0;
  }
}

void vcs_cycle() {
  while (running) {
    mos6507_fetch();
    int invalid_instruction = mos6507_decode();

    if (invalid_instruction > 0) {
      running = 0;
      printf("Instruction %x not implemented.\n", invalid_instruction);
    }
    
    tia_tick(&tia);
    tia_tick(&tia);
    tia_tick(&tia);
    
    if (tia.scanline == 0 && tia.cycle == 0) {
        hal_present(tia.frame, 160, 192);
        if (!hal_handle_events()) running = 0;
    }

    vcs_update();
  }
}

void vcs_load_rom(const char path[]) {
  FILE *file;
  uint8_t byte;
  uint16_t i;

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
  vcs_cycle();
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s file.rom\n", argv[0]);
    exit(1);
  } else {
    hal_init("VCS Emulator", 160, 192);
    vcs_load_rom(argv[1]);
    hal_cleanup();
  }

  return 0;
}
