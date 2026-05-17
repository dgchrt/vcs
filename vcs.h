#ifndef VCS_H
#define VCS_H

void vcs_update(void);
void vcs_cycle(void);
void vcs_instruction_not_implemented(int invalid_instruction);
void vcs_load_rom(const char path[]);

#endif // VCS_H
