#include "mos6507.h"
#include "bus.h"
#include "debug.h"
#include "vcs.h"

uint8_t accumulator;
uint8_t index_register_x;
uint8_t index_register_y;
uint8_t instruction;
uint8_t processor_status = 36;
uint16_t program_counter;
uint8_t registers[REGISTER_SIZE];
uint8_t running = 1;
uint16_t stack[STACK_SIZE];
uint16_t stack_pointer = 0xff;

int cycles = 0;

uint8_t mos6507_read(uint16_t address) {
    ++cycles;
    return bus_read(address);
}

void mos6507_write(uint16_t address, uint8_t value) {
    ++cycles;
    bus_write(address, value);
}

uint16_t addr_absolute() {
  /*
  Absolute

  Instructions using absolute addressing contain a full 16 bit address to
  identify the target location.
  */
  return mos6507_operand_2bytes() & BUS_MEMORY_MASK;
}

uint16_t addr_absolute_x() {
  /*
  Absolute,X

  The address to be accessed by an instruction using X register indexed absolute
  addressing is computed by taking the 16 bit address from the instruction and
  added the contents of the X register. For example if X contains $92 then an
  STA $2000,X instruction will store the accumulator at $2092 (e.g. $2000 +
  $92).
  */
  return (mos6507_operand_2bytes() + index_register_x) & BUS_MEMORY_MASK;
}

uint16_t addr_absolute_y() {
  /*
  Absolute,Y

  The Y register indexed absolute addressing mode is the same as the previous
  mode only with the contents of the Y register added to the 16 bit address from
  the instruction.
  */
  return (mos6507_operand_2bytes() + index_register_y) & BUS_MEMORY_MASK;
}

uint16_t addr_indirect() {
  /*
  Indirect

  JMP is the only 6502 instruction to support indirection.
  The instruction contains a 16 bit address which identifies the location of the
  least significant byte of another 16 bit memory address which is the real
  target of the instruction. For example if location $0120 contains $FC and
  location $0121 contains $BA then the instruction JMP ($0120) will cause the
  next instruction execution to occur at $BAFC (e.g. the contents of $0120 and
  $0121).
  */
  uint8_t byte1;
  uint8_t byte2;

  byte1 = mos6507_read(mos6507_operand_2bytes());
  byte2 = mos6507_read(mos6507_operand_2bytes() + 1);
  return (byte1 | (byte2 << BYTE_SIZE)) & BUS_MEMORY_MASK;
}

uint16_t mos6507_addr_indirect_x() {
  /*
  Indexed Indirect

  Indexed indirect addressing is normally used in conjunction with a table of
  address held on zero page. The address of the table is taken from the
  instruction and the X register added to it (with zero page wrap around) to
  give the location of the least significant byte of the target address.
  */
  uint8_t address;
  uint8_t byte1;
  uint8_t byte2;

  mos6507_fetch();
  address = instruction + index_register_x;
  byte1 = mos6507_read(address);
  byte2 = mos6507_read(address + 1);
  return (byte1 | (byte2 << BYTE_SIZE)) & BUS_MEMORY_MASK;
}

uint16_t mos6507_addr_indirect_y() {
  /*
  Indirect Indexed

  Indirect indirect addressing is the most common indirection mode used on the
  6502. In instruction contains the zero page location of the least significant
  byte of 16 bit address. The Y register is dynamically added to this value to
  generated the actual target address for operation.
  */
  uint8_t byte1;
  uint8_t byte2;

  mos6507_fetch();
  byte1 = mos6507_read(instruction);
  byte2 = mos6507_read(instruction + 1);
  return ((byte1 | (byte2 << BYTE_SIZE)) + index_register_y) & BUS_MEMORY_MASK;
}

signed char mos6507_addr_relative() {
  /*
  Relative

  Relative addressing mode is used by branch instructions (e.g. BEQ, BNE, etc.)
  which contain a signed 8 bit relative offset (e.g. -128 to +127) which is
  added to program counter if the condition is true. As the program counter
  itself is incremented during instruction execution by two the effective
  address range for the target instruction must be with -126 to +129 bytes of
  the branch.
  */
  mos6507_fetch();
  return instruction;
}

uint16_t addr_zero_page() {
  /*
  Zero Page

  An instruction using zero page addressing mode has only an 8 bit address
  operand. This limits it to addressing only the first 256 bytes of memory (e.g.
  $0000 to $00FF) where the most significant byte of the address is always zero.
  */
  mos6507_fetch();
  return instruction & BUS_MEMORY_MASK;
}

uint16_t addr_zero_page_x() {
  /*
  Zero Page,X

  The address to be accessed by an instruction using indexed zero page
  addressing is calculated by taking the 8 bit zero page address from the
  instruction and adding the current value of the X register to it. For example
  if the X register contains $0F and the instruction LDA $80,X is executed then
  the accumulator will be loaded from $008F (e.g. $80 + $0F => $8F). The address
  calculation wraps around if the sum of the base address and the register
  exceed $FF. If we repeat the last example but with $FF in the X register then
  the accumulator will be loaded from $007F (e.g. $80 + $FF => $7F) and not
  $017F.
  */
  mos6507_fetch();
  return (instruction + index_register_x) & BUS_MEMORY_MASK;
}

uint16_t addr_zero_page_y() {
  /*
  Zero Page,Y

  The address to be accessed by an instruction using indexed zero page
  addressing is calculated by taking the 8 bit zero page address from the
  instruction and adding the current value of the Y register to it. This mode
  can only be used with the LDX and STX instructions.
  */
  mos6507_fetch();
  return (instruction + index_register_y) & BUS_MEMORY_MASK;
}

void mos6507_check_negative(uint8_t data) {
  if ((data & (1 << STATUS_BIT_NEGATIVE)) > 0) {
    mos6507_flag_negative_set();
  } else {
    mos6507_flag_negative_unset();
  }
}

void mos6507_check_overflow(uint16_t data) {
  if (data > 0xff) {
    mos6507_flag_overflow_set();
  } else {
    mos6507_flag_overflow_unset();
  }
}

void mos6507_check_zero(uint8_t data) {
  if (data == 0) {
    mos6507_flag_zero_set();
  } else {
    mos6507_flag_zero_unset();
  }
}

uint8_t data_absolute() { return mos6507_read(addr_absolute()); }

uint8_t data_absolute_x() { return mos6507_read(addr_absolute_x()); }

uint8_t data_absolute_y() { return mos6507_read(addr_absolute_y()); }

uint8_t data_immediate() {
  /*
  Immediate

  Immediate addressing allows the programmer to directly specify an 8 bit
  constant within the instruction.
  */
  mos6507_fetch();
  return instruction;
}

uint8_t data_indirect_x() { return mos6507_read(mos6507_addr_indirect_x()); }

uint8_t data_indirect_y() { return mos6507_read(mos6507_addr_indirect_y()); }

uint8_t data_zero_page() { return mos6507_read(addr_zero_page()); }

uint8_t data_zero_page_x() { return mos6507_read(addr_zero_page_x()); }

uint8_t data_zero_page_y() { return mos6507_read(addr_zero_page_y()); }

int mos6507_decode() {
  cycles = 0;

  switch (instruction) {
  case 0x00:
    mos6507_instruction_brk();
    break;

  case 0x01:
    mos6507_instruction_ora(data_indirect_x());
    break;

  case 0x05:
    mos6507_instruction_ora(data_zero_page());
    break;

  case 0x06:
    mos6507_instruction_asl(addr_zero_page());
    break;

  case 0x03:
    mos6507_instruction_slo(mos6507_addr_indirect_x());
    break;

  case 0x07:
    mos6507_instruction_slo(addr_zero_page());
    break;

  case 0x0f:
    mos6507_instruction_slo(addr_absolute());
    break;

  case 0x13:
    mos6507_instruction_slo(mos6507_addr_indirect_y());
    break;

  case 0x17:
    mos6507_instruction_slo(addr_zero_page_x());
    break;

  case 0x1b:
    mos6507_instruction_slo(addr_absolute_y());
    break;

  case 0x1f:
    mos6507_instruction_slo(addr_absolute_x());
    break;

  case 0x08:
    mos6507_instruction_php();
    break;

  case 0x09:
    mos6507_instruction_ora(data_immediate());
    break;

  case 0x0b:
    mos6507_instruction_anc(data_immediate());
    break;

  case 0x0a:
    mos6507_instruction_asl(0);
    break;

  case 0x0d:
    mos6507_instruction_ora(data_absolute());
    break;

  case 0x0e:
    mos6507_instruction_asl(addr_absolute());
    break;

  case 0x10:
    mos6507_instruction_bpl(mos6507_addr_relative());
    break;

  case 0x11:
    mos6507_instruction_ora(data_indirect_y());
    break;

  case 0x15:
    mos6507_instruction_ora(data_zero_page_x());
    break;

  case 0x16:
    mos6507_instruction_asl(addr_zero_page_x());
    break;

  case 0x18:
    mos6507_instruction_clc();
    break;

  case 0x19:
    mos6507_instruction_ora(data_absolute_y());
    break;

  case 0x1d:
    mos6507_instruction_ora(data_absolute_x());
    break;

  case 0x1e:
    mos6507_instruction_asl(addr_absolute_x());
    break;

  case 0x20:
    mos6507_instruction_jsr(addr_absolute());
    break;

  case 0x21:
    mos6507_instruction_and(data_indirect_x());
    break;

  case 0x23:
    mos6507_instruction_rla(mos6507_addr_indirect_x());
    break;

  case 0x24:
    mos6507_instruction_bit(data_zero_page());
    break;

  case 0x25:
    mos6507_instruction_and(data_zero_page());
    break;

  case 0x26:
    mos6507_instruction_rol(addr_zero_page());
    break;

  case 0x27:
    mos6507_instruction_rla(addr_zero_page());
    break;

  case 0x2f:
    mos6507_instruction_rla(addr_absolute());
    break;

  case 0x33:
    mos6507_instruction_rla(mos6507_addr_indirect_y());
    break;

  case 0x37:
    mos6507_instruction_rla(addr_zero_page_x());
    break;

  case 0x3b:
    mos6507_instruction_rla(addr_absolute_y());
    break;

  case 0x3f:
    mos6507_instruction_rla(addr_absolute_x());
    break;

  case 0x28:
    mos6507_instruction_plp();
    break;

  case 0x29:
    mos6507_instruction_and(data_immediate());
    break;

  case 0x2b:
    mos6507_instruction_anc(data_immediate());
    break;

  case 0x2a:
    mos6507_instruction_rol(0);
    break;

  case 0x2c:
    mos6507_instruction_bit(data_absolute());
    break;

  case 0x2d:
    mos6507_instruction_and(data_absolute());
    break;

  case 0x2e:
    mos6507_instruction_rol(addr_absolute());
    break;

  case 0x30:
    mos6507_instruction_bmi(mos6507_addr_relative());
    break;

  case 0x31:
    mos6507_instruction_and(data_indirect_y());
    break;

  case 0x35:
    mos6507_instruction_and(data_zero_page_x());
    break;

  case 0x36:
    mos6507_instruction_rol(addr_zero_page_x());
    break;

  case 0x38:
    mos6507_instruction_sec();
    break;

  case 0x39:
    mos6507_instruction_and(data_absolute_y());
    break;

  case 0x3d:
    mos6507_instruction_and(data_absolute_x());
    break;

  case 0x3e:
    mos6507_instruction_rol(addr_absolute_x());
    break;

  case 0x40:
    mos6507_instruction_rti();
    break;

  case 0x41:
    mos6507_instruction_eor(data_indirect_x());
    break;

  case 0x43:
    mos6507_instruction_sre(mos6507_addr_indirect_x());
    break;

  case 0x45:
    mos6507_instruction_eor(data_zero_page());
    break;

  case 0x46:
    mos6507_instruction_lsr(addr_zero_page());
    break;

  case 0x47:
    mos6507_instruction_sre(addr_zero_page());
    break;

  case 0x4f:
    mos6507_instruction_sre(addr_absolute());
    break;

  case 0x53:
    mos6507_instruction_sre(mos6507_addr_indirect_y());
    break;

  case 0x57:
    mos6507_instruction_sre(addr_zero_page_x());
    break;

  case 0x5b:
    mos6507_instruction_sre(addr_absolute_y());
    break;

  case 0x5f:
    mos6507_instruction_sre(addr_absolute_x());
    break;

  case 0x48:
    mos6507_instruction_pha();
    break;

  case 0x49:
    mos6507_instruction_eor(data_immediate());
    break;

  case 0x4b:
    mos6507_instruction_alr(data_immediate());
    break;

  case 0x4a:
    mos6507_instruction_lsr(0);
    break;

  case 0x4c:
    mos6507_instruction_jmp(addr_absolute());
    break;

  case 0x4d:
    mos6507_instruction_eor(data_absolute());
    break;

  case 0x4e:
    mos6507_instruction_lsr(addr_absolute());
    break;

  case 0x50:
    mos6507_instruction_bvc(mos6507_addr_relative());
    break;

  case 0x51:
    mos6507_instruction_eor(data_indirect_y());
    break;

  case 0x55:
    mos6507_instruction_eor(data_zero_page_x());
    break;

  case 0x56:
    mos6507_instruction_lsr(addr_zero_page_x());
    break;

  case 0x58:
    mos6507_instruction_cli();
    break;

  case 0x59:
    mos6507_instruction_eor(data_absolute_y());
    break;

  case 0x5d:
    mos6507_instruction_eor(data_absolute_x());
    break;

  case 0x5e:
    mos6507_instruction_lsr(addr_absolute_x());
    break;

  case 0x60:
    mos6507_instruction_rts();
    break;

  case 0x61:
    mos6507_instruction_adc(data_indirect_x());
    break;

  case 0x63:
    mos6507_instruction_rra(mos6507_addr_indirect_x());
    break;

  case 0x65:
    mos6507_instruction_adc(data_zero_page());
    break;

  case 0x66:
    mos6507_instruction_ror(addr_zero_page());
    break;

  case 0x67:
    mos6507_instruction_rra(addr_zero_page());
    break;

  case 0x6f:
    mos6507_instruction_rra(addr_absolute());
    break;

  case 0x73:
    mos6507_instruction_rra(mos6507_addr_indirect_y());
    break;

  case 0x77:
    mos6507_instruction_rra(addr_zero_page_x());
    break;

  case 0x7b:
    mos6507_instruction_rra(addr_absolute_y());
    break;

  case 0x7f:
    mos6507_instruction_rra(addr_absolute_x());
    break;

  case 0x68:
    mos6507_instruction_pla();
    break;

  case 0x69:
    mos6507_instruction_adc(data_immediate());
    break;

  case 0x6a:
    mos6507_instruction_ror(0);
    break;

  case 0x6c:
    mos6507_instruction_jmp(addr_indirect());
    break;

  case 0x6d:
    mos6507_instruction_adc(data_absolute());
    break;

  case 0x6e:
    mos6507_instruction_ror(addr_absolute());
    break;

  case 0x70:
    mos6507_instruction_bvs(mos6507_addr_relative());
    break;

  case 0x71:
    mos6507_instruction_adc(data_indirect_y());
    break;

  case 0x75:
    mos6507_instruction_adc(data_zero_page_x());
    break;

  case 0x76:
    mos6507_instruction_ror(addr_zero_page_x());
    break;

  case 0x78:
    mos6507_instruction_sei();
    break;

  case 0x79:
    mos6507_instruction_adc(data_absolute_y());
    break;

  case 0x7d:
    mos6507_instruction_adc(data_absolute_x());
    break;

  case 0x7e:
    mos6507_instruction_ror(addr_absolute_x());
    break;

  case 0x81:
    mos6507_instruction_sta(mos6507_addr_indirect_x());
    break;

  case 0x83:
    mos6507_instruction_sax(mos6507_addr_indirect_x());
    break;

  case 0x87:
    mos6507_instruction_sax(addr_zero_page());
    break;

  case 0x8f:
    mos6507_instruction_sax(addr_absolute());
    break;

  case 0x97:
    mos6507_instruction_sax(addr_zero_page_y());
    break;

  case 0x84:
    mos6507_instruction_sty(addr_zero_page());
    break;

  case 0x85:
    mos6507_instruction_sta(addr_zero_page());
    break;

  case 0x86:
    mos6507_instruction_stx(addr_zero_page());
    break;

  case 0x88:
    mos6507_instruction_dey();
    break;

  case 0x8a:
    mos6507_instruction_txa();
    break;

  case 0x8b:
    mos6507_instruction_ane(data_immediate());
    break;

  case 0x8c:
    mos6507_instruction_sty(addr_absolute());
    break;

  case 0x8d:
    mos6507_instruction_sta(addr_absolute());
    break;

  case 0x8e:
    mos6507_instruction_stx(addr_absolute());
    break;

  case 0x90:
    mos6507_instruction_bcc(mos6507_addr_relative());
    break;

  case 0x91:
    mos6507_instruction_sta(mos6507_addr_indirect_y());
    break;

  case 0x93:
    mos6507_instruction_sha(mos6507_addr_indirect_y());
    break;

  case 0x94:
    mos6507_instruction_sty(addr_zero_page_x());
    break;

  case 0x95:
    mos6507_instruction_sta(addr_zero_page_x());
    break;

  case 0x96:
    mos6507_instruction_stx(addr_zero_page_y());
    break;

  case 0x98:
    mos6507_instruction_tya();
    break;

  case 0x99:
    mos6507_instruction_sta(addr_absolute_y());
    break;

  case 0x9a:
    mos6507_instruction_txs();
    break;

  case 0x9d:
    mos6507_instruction_sta(addr_absolute_x());
    break;

  case 0x9b:
    mos6507_instruction_tas(addr_absolute_y());
    break;

  case 0x9f:
    mos6507_instruction_sha(addr_absolute_x());
    break;

  case 0xa0:
    mos6507_instruction_ldy(data_immediate());
    break;

  case 0xa1:
    mos6507_instruction_lda(data_indirect_x());
    break;

  case 0xa2:
    mos6507_instruction_ldx(data_immediate());
    break;

  case 0xa3:
    mos6507_instruction_lax(mos6507_read(mos6507_addr_indirect_x()));
    break;

  case 0xa7:
    mos6507_instruction_lax(mos6507_read(addr_zero_page()));
    break;

  case 0xaf:
    mos6507_instruction_lax(mos6507_read(addr_absolute()));
    break;

  case 0xb3:
    mos6507_instruction_lax(mos6507_read(mos6507_addr_indirect_y()));
    break;

  case 0xb7:
    mos6507_instruction_lax(mos6507_read(addr_zero_page_y()));
    break;

  case 0xbf:
    mos6507_instruction_lax(mos6507_read(addr_absolute_y()));
    break;

  case 0xa4:
    mos6507_instruction_ldy(data_zero_page());
    break;

  case 0xa5:
    mos6507_instruction_lda(data_zero_page());
    break;

  case 0xa6:
    mos6507_instruction_ldx(data_zero_page());
    break;

  case 0xa8:
    mos6507_instruction_tay();
    break;

  case 0xa9:
    mos6507_instruction_lda(data_immediate());
    break;

  case 0xaa:
    mos6507_instruction_tax();
    break;

  case 0xac:
    mos6507_instruction_ldy(data_absolute());
    break;

  case 0xad:
    mos6507_instruction_lda(data_absolute());
    break;

  case 0xae:
    mos6507_instruction_ldx(data_absolute());
    break;

  case 0xb0:
    mos6507_instruction_bcs(mos6507_addr_relative());
    break;

  case 0xb1:
    mos6507_instruction_lda(data_indirect_y());
    break;

  case 0xb4:
    mos6507_instruction_ldy(data_zero_page_x());
    break;

  case 0xb5:
    mos6507_instruction_lda(data_zero_page_x());
    break;

  case 0xb6:
    mos6507_instruction_ldx(data_zero_page_y());
    break;

  case 0xb8:
    mos6507_instruction_clv();
    break;

  case 0xb9:
    mos6507_instruction_lda(data_absolute_y());
    break;

  case 0xba:
    mos6507_instruction_tsx();
    break;

  case 0xbb:
    mos6507_instruction_las(data_absolute_y());
    break;

  case 0xbc:
    mos6507_instruction_ldy(data_absolute_x());
    break;

  case 0xbd:
    mos6507_instruction_lda(data_absolute_x());
    break;

  case 0xbe:
    mos6507_instruction_ldx(data_absolute_y());
    break;

  case 0xc0:
    mos6507_instruction_cpy(data_immediate());
    break;

  case 0xc1:
    mos6507_instruction_cmp(data_indirect_x());
    break;

  case 0xc3:
    mos6507_instruction_dcp(mos6507_addr_indirect_x());
    break;

  case 0xc4:
    mos6507_instruction_cpy(data_zero_page());
    break;

  case 0xc5:
    mos6507_instruction_cmp(data_zero_page());
    break;

  case 0xc6:
    mos6507_instruction_dec(addr_zero_page());
    break;

  case 0xc7:
    mos6507_instruction_dcp(addr_zero_page());
    break;

  case 0xc8:
    mos6507_instruction_iny();
    break;

  case 0xc9:
    mos6507_instruction_cmp(data_immediate());
    break;

  case 0xca:
    mos6507_instruction_dex();
    break;

  case 0xcb:
    mos6507_instruction_sbx(data_immediate());
    break;

  case 0xcc:
    mos6507_instruction_cpy(data_absolute());
    break;

  case 0xcd:
    mos6507_instruction_cmp(data_absolute());
    break;

  case 0xce:
    mos6507_instruction_dec(addr_absolute());
    break;

  case 0xcf:
    mos6507_instruction_dcp(addr_absolute());
    break;

  case 0xd0:
    mos6507_instruction_bne(mos6507_addr_relative());
    break;

  case 0xd1:
    mos6507_instruction_cmp(data_indirect_y());
    break;

  case 0xd3:
    mos6507_instruction_dcp(mos6507_addr_indirect_y());
    break;

  case 0xd5:
    mos6507_instruction_cmp(data_zero_page_x());
    break;

  case 0xd6:
    mos6507_instruction_dec(addr_zero_page_x());
    break;

  case 0xd7:
    mos6507_instruction_dcp(addr_zero_page_x());
    break;

  case 0xd8:
    mos6507_instruction_cld();
    break;

  case 0xd9:
    mos6507_instruction_cmp(data_absolute_y());
    break;

  case 0xdb:
    mos6507_instruction_dcp(addr_absolute_y());
    break;

  case 0xdd:
    mos6507_instruction_cmp(data_absolute_x());
    break;

  case 0xde:
    mos6507_instruction_dec(addr_absolute_x());
    break;

  case 0xdf:
    mos6507_instruction_dcp(addr_absolute_x());
    break;

  case 0xe0:
    mos6507_instruction_cpx(data_immediate());
    break;

  case 0xe1:
    mos6507_instruction_sbc(data_indirect_x());
    break;

  case 0xe3:
    mos6507_instruction_isc(mos6507_addr_indirect_x());
    break;

  case 0xe4:
    mos6507_instruction_cpx(data_zero_page());
    break;

  case 0xe5:
    mos6507_instruction_sbc(data_zero_page());
    break;

  case 0xe6:
    mos6507_instruction_inc(addr_zero_page());
    break;

  case 0xe7:
    mos6507_instruction_isc(addr_zero_page());
    break;

  case 0xe8:
    mos6507_instruction_inx();
    break;

  case 0xe9:
    mos6507_instruction_sbc(data_immediate());
    break;

  case 0xea:
    mos6507_instruction_nop();
    break;

  case 0xec:
    mos6507_instruction_cpx(data_absolute());
    break;

  case 0xed:
    mos6507_instruction_sbc(data_absolute());
    break;

  case 0xee:
    mos6507_instruction_inc(addr_absolute());
    break;

  case 0xef:
    mos6507_instruction_isc(addr_absolute());
    break;

  case 0xf0:
    mos6507_instruction_beq(mos6507_addr_relative());
    break;

  case 0xf1:
    mos6507_instruction_sbc(data_indirect_y());
    break;

  case 0xf3:
    mos6507_instruction_isc(mos6507_addr_indirect_y());
    break;

  case 0xf5:
    mos6507_instruction_sbc(data_zero_page_x());
    break;

  case 0xf6:
    mos6507_instruction_inc(addr_zero_page_x());
    break;

  case 0xf7:
    mos6507_instruction_isc(addr_zero_page_x());
    break;

  case 0xf8:
    mos6507_instruction_sed();
    break;

  case 0xf9:
    mos6507_instruction_sbc(data_absolute_y());
    break;

  case 0xfb:
    mos6507_instruction_isc(addr_absolute_y());
    break;

  case 0xfd:
    mos6507_instruction_sbc(data_absolute_x());
    break;

  case 0xfe:
    mos6507_instruction_inc(addr_absolute_x());
    break;

  case 0xff:
    mos6507_instruction_isc(addr_absolute_x());
    break;

  default:
    vcs_instruction_not_implemented(instruction);
    break;
  }

  return cycles;
}

void mos6507_fetch() {
  instruction = mos6507_read(program_counter++);
  debug_update(program_counter - 1, instruction, accumulator, index_register_x,
               index_register_y, stack_pointer, processor_status, cycles);
}

uint8_t mos6507_flag_break() {
  return mos6507_processor_status_flag(STATUS_BIT_BREAK);
}

void mos6507_flag_break_set() {
  mos6507_processor_status_flag_set(STATUS_BIT_BREAK);
}

void mos6507_flag_break_unset() {
  mos6507_processor_status_flag_unset(STATUS_BIT_BREAK);
}

uint8_t mos6507_flag_carry() {
  return mos6507_processor_status_flag(STATUS_BIT_CARRY);
}

void mos6507_flag_carry_set() {
  mos6507_processor_status_flag_set(STATUS_BIT_CARRY);
}

void mos6507_flag_carry_unset() {
  mos6507_processor_status_flag_unset(STATUS_BIT_CARRY);
}

uint8_t mos6507_flag_decimal() {
  return mos6507_processor_status_flag(STATUS_BIT_DECIMAL);
}

void mos6507_flag_decimal_set() {
  mos6507_processor_status_flag_set(STATUS_BIT_DECIMAL);
}

void mos6507_flag_decimal_unset() {
  mos6507_processor_status_flag_unset(STATUS_BIT_DECIMAL);
}

uint8_t mos6507_flag_interrupt() {
  return mos6507_processor_status_flag(STATUS_BIT_INTERRUPT);
}

void mos6507_flag_interrupt_set() {
  mos6507_processor_status_flag_set(STATUS_BIT_INTERRUPT);
}

void mos6507_flag_interrupt_unset() {
  mos6507_processor_status_flag_unset(STATUS_BIT_INTERRUPT);
}

uint8_t mos6507_flag_negative() {
  return mos6507_processor_status_flag(STATUS_BIT_NEGATIVE);
}

void mos6507_flag_negative_set() {
  mos6507_processor_status_flag_set(STATUS_BIT_NEGATIVE);
}

void mos6507_flag_negative_unset() {
  mos6507_processor_status_flag_unset(STATUS_BIT_NEGATIVE);
}

uint8_t mos6507_flag_overflow() {
  return mos6507_processor_status_flag(STATUS_BIT_OVERFLOW);
}

void mos6507_flag_overflow_set() {
  mos6507_processor_status_flag_set(STATUS_BIT_OVERFLOW);
}

void mos6507_flag_overflow_unset() {
  mos6507_processor_status_flag_unset(STATUS_BIT_OVERFLOW);
}

uint8_t mos6507_flag_zero() {
  return mos6507_processor_status_flag(STATUS_BIT_ZERO);
}

void mos6507_flag_zero_set() {
  mos6507_processor_status_flag_set(STATUS_BIT_ZERO);
}

void mos6507_flag_zero_unset() {
  mos6507_processor_status_flag_unset(STATUS_BIT_ZERO);
}

void mos6507_instruction_adc(uint8_t data) {
  /*
  ADC - Add with Carry

  A,Z,C,N = A+M+C
  This instruction adds the contents of a memory location to the accumulator
  together with the carry bit. If overflow occurs the carry bit is set, this
  enables multiple byte addition to be performed.
  */
  uint16_t sum = accumulator + data + mos6507_flag_carry();
  accumulator = sum;
  mos6507_check_negative(accumulator);
  mos6507_check_overflow(sum);
  mos6507_check_zero(accumulator);
}

void mos6507_instruction_alr(uint8_t data) {
  /*
  ALR - AND Memory with Accumulator and LSR (Immediate)

  A = (A & M) >> 1, C = bit 0 of (A & M)
  AND memory with accumulator, then shift right.
  */
  accumulator &= data;
  if (accumulator & 1)
    mos6507_flag_carry_set();
  else
    mos6507_flag_carry_unset();
  accumulator >>= 1;
  mos6507_check_negative(accumulator);
  mos6507_check_zero(accumulator);
}

void mos6507_instruction_anc(uint8_t data) {
  /*
  ANC - AND Memory with Accumulator (Immediate)

  A = A & M, C = bit 7 of A
  AND memory with accumulator, then set carry to bit 7 of result.
  */
  accumulator &= data;
  mos6507_check_negative(accumulator);
  mos6507_check_zero(accumulator);
  if (accumulator & 0x80)
    mos6507_flag_carry_set();
  else
    mos6507_flag_carry_unset();
}

void mos6507_instruction_and(uint8_t data) {
  /*
  AND - Logical AND

  A,Z,N = A&M
  A logical AND is performed, bit by bit, on the accumulator contents using the
  contents of a byte of memory.
  */
  accumulator &= data;
  mos6507_check_negative(data);
  mos6507_check_zero(data);
}

void mos6507_instruction_ane(uint8_t data) {
  /*
  ANE - AND X Register with Accumulator and Memory (Immediate)

  A = (A | constant) & X & M
  AND accumulator with memory and X register, with a magic constant.
  */
  accumulator = (accumulator | 0xee) & index_register_x & data;
  mos6507_check_negative(accumulator);
  mos6507_check_zero(accumulator);
}

void mos6507_instruction_asl(uint16_t addr) {
  /*
  ASL - Arithmetic Shift Left

  A,Z,C,N = M*2 or M,Z,C,N = M*2
  This operation shifts all the bits of the accumulator or memory contents one
  bit left. Bit 0 is set to 0 and bit 7 is placed in the carry flag. The effect
  of this operation is to multiply the memory contents by 2 (ignoring 2's
  complement considerations), setting the carry if the result will not fit in 8
  bits.
  */
  uint8_t data;

  if (addr) {
    data = mos6507_read(addr);
  } else {
    data = accumulator;
  }

  if (data >> 7) {
    mos6507_flag_carry_set();
  } else {
    mos6507_flag_carry_unset();
  }

  data <<= 1;
  mos6507_check_negative(data);
  mos6507_check_zero(data);

  if (addr) {
    mos6507_write(addr, data);
  } else {
    accumulator = data;
  }
}

void mos6507_instruction_bcc(signed char addr) {
  /*
  BCC - Branch if Carry Clear

  If the carry flag is clear then add the relative displacement to the program
  counter to cause a branch to a new location.
  */
  if (!mos6507_flag_carry()) {
    program_counter += addr;
  }
}

void mos6507_instruction_bcs(signed char addr) {
  /*
  BCS - Branch if Carry Set

  If the carry flag is set then add the relative displacement to the program
  counter to cause a branch to a new location.
  */
  if (mos6507_flag_carry()) {
    program_counter += addr;
  }
}

void mos6507_instruction_beq(signed char addr) {
  /*
  BEQ - Branch if Equal

  If the zero flag is set then add the relative displacement to the program
  counter to cause a branch to a new location.
  */
  if (mos6507_flag_zero()) {
    program_counter += addr;
  }
}

void mos6507_instruction_bit(uint8_t data) {
  /*
  BIT - Bit Test

  A & M, N = M7, V = M6
  This instructions is used to test if one or more bits are set in a target
  memory location. The mask pattern in A is ANDed with the value in memory to
  set or clear the zero flag, but the result is not kept. Bits 7 and 6 of the
  value from memory are copied into the N and V flags.
  */
  uint8_t result = accumulator & data;

  if ((result >> STATUS_BIT_NEGATIVE) & 1) {
    mos6507_flag_negative_set();
  } else {
    mos6507_flag_negative_unset();
  }

  if ((result >> STATUS_BIT_OVERFLOW) & 1) {
    mos6507_flag_overflow_set();
  } else {
    mos6507_flag_overflow_unset();
  }
}

void mos6507_instruction_bmi(signed char addr) {
  /*
  BMI - Branch if Minus

  If the negative flag is set then add the relative displacement to the program
  counter to cause a branch to a new location.
  */
  if (mos6507_flag_negative()) {
    program_counter += addr;
  }
}

void mos6507_instruction_bne(signed char addr) {
  /*
  BNE - Branch if Not Equal

  If the zero flag is clear then add the relative displacement to the program
  counter to cause a branch to a new location.
  */
  if (!mos6507_flag_negative()) {
    program_counter += addr;
  }
}

void mos6507_instruction_bpl(signed char addr) {
  /*
  BPL - Branch if Positive

  If the negative flag is clear then add the relative displacement to the
  program counter to cause a branch to a new location.
  */
  if (mos6507_flag_negative()) {
    program_counter += addr;
  }
}

void mos6507_instruction_brk() {
  /*
  BRK - Force Interrupt

  The BRK instruction forces the generation of an interrupt request.
  The program counter and processor status are pushed on the stack then the IRQ
  interrupt vector at $FFFE/F is loaded into the PC and the break flag in the
  status set to one.
  */
  mos6507_stack_push(program_counter);
  mos6507_stack_push(program_counter >> BYTE_SIZE);
  mos6507_stack_push(processor_status);
  program_counter =
      (mos6507_read(VECTOR_IRQ) | (mos6507_read(VECTOR_IRQ + 1) << BYTE_SIZE)) &
      BUS_MEMORY_MASK;
  mos6507_flag_break_set();
}

void mos6507_instruction_bvc(signed char addr) {
  /*
  BVC - Branch if Overflow Clear

  If the overflow flag is clear then add the relative displacement to the
  program counter to cause a branch to a new location.
  */
  if (!mos6507_flag_overflow()) {
    program_counter += addr;
  }
}

void mos6507_instruction_bvs(signed char addr) {
  /*
  BVS - Branch if Overflow Set

  If the overflow flag is set then add the relative displacement to the program
  counter to cause a branch to a new location.
  */
  if (mos6507_flag_overflow()) {
    program_counter += addr;
  }
}

void mos6507_instruction_clc() {
  /*
  CLC - Clear Carry Flag

  C = 0
  Set the carry flag to zero.
  */
  mos6507_flag_carry_unset();
}

void mos6507_instruction_cld() {
  /*
  CLD - Clear Decimal Mode

  D = 0
  Sets the decimal mode flag to zero.
  */
  mos6507_flag_decimal_unset();
}

void mos6507_instruction_cli() {
  /*
  CLI - Clear Interrupt Disable

  I = 0
  Clears the interrupt disable flag allowing normal interrupt requests to be
  serviced.
  */
  mos6507_flag_interrupt_unset();
}

void mos6507_instruction_clv() {
  /*
  CLV - Clear Overflow Flag

  V = 0
  Clears the overflow flag.
  */
  mos6507_flag_overflow_unset();
}

void mos6507_instruction_cmp(uint8_t data) {
  /*
  CMP - Compare

  Z,C,N = A-M
  This instruction compares the contents of the accumulator with another memory
  held value and sets the zero and carry flags as appropriate.
  */
  uint8_t result = accumulator - data;
  mos6507_check_negative(result);
  mos6507_check_zero(result);

  if (accumulator < data) {
    mos6507_flag_carry_unset();
  } else {
    mos6507_flag_carry_set();
  }
}

void mos6507_instruction_cpx(uint8_t data) {
  /*
  CPX - Compare X Register

  Z,C,N = X-M
  This instruction compares the contents of the X register with another memory
  held value and sets the zero and carry flags as appropriate.
  */
  uint8_t result = index_register_x - data;
  mos6507_check_negative(result);
  mos6507_check_zero(result);

  if (index_register_x < data) {
    mos6507_flag_carry_unset();
  } else {
    mos6507_flag_carry_set();
  }
}

void mos6507_instruction_cpy(uint8_t data) {
  /*
  CPY - Compare Y Register

  Z,C,N = Y-M
  This instruction compares the contents of the Y register with another memory
  held value and sets the zero and carry flags as appropriate.
  */
  uint8_t result = index_register_y - data;
  mos6507_check_negative(result);
  mos6507_check_zero(result);

  if (index_register_y < data) {
    mos6507_flag_carry_unset();
  } else {
    mos6507_flag_carry_set();
  }
}

void mos6507_instruction_dcp(uint16_t addr) {
  /*
  DCP - Decrement Memory and Compare

  M = M - 1, A - M
  Decrements memory, then compares accumulator with memory.
  */
  mos6507_instruction_dec(addr);
  mos6507_instruction_cmp(mos6507_read(addr));
}

void mos6507_instruction_dec(uint16_t addr) {
  /*
  DEC - Decrement Memory

  M,Z,N = M-1
  Subtracts one from the value held at a specified memory location setting the
  zero and negative flags as appropriate.
  */
  uint8_t data = mos6507_read(addr);
  --data;
  mos6507_write(addr, data);
  mos6507_check_negative(data);
  mos6507_check_zero(data);
}

void mos6507_instruction_dex() {
  /*
  DEX - Decrement X Register

  X,Z,N = X-1
  Subtracts one from the X register setting the zero and negative flags as
  appropriate.
  */
  --index_register_x;
  mos6507_check_negative(index_register_x);
  mos6507_check_zero(index_register_x);
}

void mos6507_instruction_dey() {
  /*
  DEY - Decrement Y Register

  Y,Z,N = Y-1
  Subtracts one from the Y register setting the zero and negative flags as
  appropriate.
  */
  --index_register_y;
  mos6507_check_negative(index_register_y);
  mos6507_check_zero(index_register_y);
}

void mos6507_instruction_eor(uint8_t data) {
  /*
  EOR - Exclusive OR

  A,Z,N = A^M
  An exclusive OR is performed, bit by bit, on the accumulator contents using
  the contents of a byte of memory.
  */
  accumulator ^= data;
  mos6507_check_negative(accumulator);
  mos6507_check_zero(accumulator);
}

void mos6507_instruction_inc(uint16_t addr) {
  /*
  INC - Increment Memory

  M,Z,N = M+1
  Adds one to the value held at a specified memory location setting the zero and
  negative flags as appropriate.
  */
  uint8_t data = mos6507_read(addr);
  ++data;
  mos6507_write(addr, data);
  mos6507_check_negative(data);
  mos6507_check_zero(data);
}

void mos6507_instruction_inx() {
  /*
  INX - Increment X Register

  X,Z,N = X+1
  Adds one to the X register setting the zero and negative flags as appropriate.
  */
  ++index_register_x;
  mos6507_check_negative(index_register_x);
  mos6507_check_zero(index_register_x);
}

void mos6507_instruction_iny() {
  /*
  INY - Increment Y Register

  Y,Z,N = Y+1
  Adds one to the Y register setting the zero and negative flags as appropriate.
  */
  ++index_register_y;
  mos6507_check_negative(index_register_y);
  mos6507_check_zero(index_register_y);
}

void mos6507_instruction_isc(uint16_t addr) {
  /*
  ISC - Increment Memory and Subtract with Carry

  M = M + 1, A = A - M - (1 - C)
  Increments memory, then subtracts memory from accumulator with borrow.
  */
  mos6507_instruction_inc(addr);
  mos6507_instruction_sbc(mos6507_read(addr));
}

void mos6507_instruction_jmp(uint16_t addr) {
  /*
  JMP - Jump

  Sets the program counter to the address specified by the operand.
  */
  program_counter = addr;
}

void mos6507_instruction_jsr(uint16_t addr) {
  /*
  JSR - Jump to Subroutine

  The JSR instruction pushes the address (minus one) of the return point on to
  the stack and then sets the program counter to the target memory address.
  */
  mos6507_stack_push(program_counter);
  mos6507_stack_push(program_counter >> BYTE_SIZE);
  program_counter = addr;
}

void mos6507_instruction_las(uint8_t data) {
  /*
  LAS - Load Accumulator, X Register, and Stack Pointer

  A = X = S = (M & S)
  */
  accumulator = index_register_x = stack_pointer = (data & stack_pointer);
  mos6507_check_negative(accumulator);
  mos6507_check_zero(accumulator);
}

void mos6507_instruction_lax(uint8_t data) {
  /*
  LAX - Load Accumulator and X Register

  A, X = M
  Loads both the accumulator and X register with the value from memory.
  */
  mos6507_instruction_lda(data);
  mos6507_instruction_ldx(data);
}

void mos6507_instruction_lda(uint8_t data) {
  /*
  LDA - Load Accumulator

  A,Z,N = M
  Loads a byte of memory into the accumulator setting the zero and negative
  flags as appropriate.
  */
  accumulator = data;
  mos6507_check_negative(accumulator);
  mos6507_check_zero(accumulator);
}

void mos6507_instruction_ldx(uint8_t data) {
  /*
  LDX - Load X Register

  X,Z,N = M
  Loads a byte of memory into the X register setting the zero and negative flags
  as appropriate.
  */
  index_register_x = data;
  mos6507_check_negative(index_register_x);
  mos6507_check_zero(index_register_x);
}

void mos6507_instruction_ldy(uint8_t data) {
  /*
  LDY - Load Y Register

  Y,Z,N = M
  Loads a byte of memory into the Y register setting the zero and negative flags
  as appropriate.
  */
  index_register_y = data;
  mos6507_check_negative(index_register_y);
  mos6507_check_zero(index_register_y);
}

void mos6507_instruction_lsr(uint16_t addr) {
  /*
  LSR - Logical Shift Right

  A,C,Z,N = A/2 or M,C,Z,N = M/2
  Each of the bits in A or M is shift one place to the right. The bit that was
  in bit 0 is shifted into the carry flag. Bit 7 is set to zero.
  */
  uint8_t data;

  if (addr) {
    data = mos6507_read(addr);
  } else {
    data = accumulator;
  }

  if (data & 1) {
    mos6507_flag_carry_set();
  } else {
    mos6507_flag_carry_unset();
  }

  data >>= 1;
  mos6507_check_negative(data);
  mos6507_check_zero(data);

  if (addr) {
    mos6507_write(addr, data);
  } else {
    accumulator = data;
  }
}

void mos6507_instruction_nop() {
  /*
  NOP - No Operation

  The NOP instruction causes no changes to the processor other than the normal
  incrementing of the program counter to the next instruction.
  */
}

void mos6507_instruction_ora(uint8_t data) {
  /*
  ORA - Logical Inclusive OR

  A,Z,N = A|M
  An inclusive OR is performed, bit by bit, on the accumulator contents using
  the contents of a byte of memory.
  */
  accumulator |= data;
  mos6507_check_negative(accumulator);
  mos6507_check_zero(accumulator);
}

void mos6507_instruction_pha() {
  /*
  PHA - Push Accumulator

  Pushes a copy of the accumulator on to the stack.
  */
  mos6507_stack_push(accumulator);
}

void mos6507_instruction_php() {
  /*
  PHP - Push Processor Status

  Pushes a copy of the status flags on to the stack.
  */
  mos6507_stack_push(processor_status);
}

void mos6507_instruction_pla() {
  /*
  PLA - Pull Accumulator

  Pulls an 8 bit value from the stack and into the accumulator.
  The zero and negative flags are set as appropriate.
  */
  accumulator = mos6507_stack_pull();
  mos6507_check_negative(accumulator);
  mos6507_check_zero(accumulator);
}

void mos6507_instruction_plp() {
  /*
  PLP - Pull Processor Status

  Pulls an 8 bit value from the stack and into the processor flags.
  The flags will take on new states as determined by the value pulled.
  */
  processor_status = mos6507_stack_pull();
}

void mos6507_instruction_rla(uint16_t addr) {
  /*
  RLA - Rotate Left and AND

  M = M << 1 | C, A = A & M
  Rotates memory left, then ANDs accumulator with memory.
  */
  mos6507_instruction_rol(addr);
  mos6507_instruction_and(mos6507_read(addr));
}

void mos6507_instruction_rol(uint16_t addr) {
  /*
  ROL - Rotate Left

  Move each of the bits in either A or M one place to the left.
  Bit 0 is filled with the current value of the carry flag whilst the old bit 7
  becomes the new carry flag value.
  */
  uint8_t carry_in = mos6507_flag_carry();
  uint8_t data;

  if (addr) {
    data = mos6507_read(addr);
  } else {
    data = accumulator;
  }

  if (data >> 7) {
    mos6507_flag_carry_set();
  } else {
    mos6507_flag_carry_unset();
  }

  data <<= 1;
  data |= carry_in;
  mos6507_check_negative(data);
  mos6507_check_zero(data);

  if (addr) {
    mos6507_write(addr, data);
  } else {
    accumulator = data;
  }
}

void mos6507_instruction_ror(uint16_t addr) {
  /*
  ROR - Rotate Right

  Move each of the bits in either A or M one place to the right.
  Bit 7 is filled with the current value of the carry flag whilst the old bit 0
  becomes the new carry flag value.
  */
  uint8_t carry_in = mos6507_flag_carry();
  uint8_t data;

  if (addr) {
    data = mos6507_read(addr);
  } else {
    data = accumulator;
  }

  if (data & 1) {
    mos6507_flag_carry_set();
  } else {
    mos6507_flag_carry_unset();
  }

  data >>= 1;
  data |= (carry_in << 7);
  mos6507_check_negative(data);
  mos6507_check_zero(data);

  if (addr) {
    mos6507_write(addr, data);
  } else {
    accumulator = data;
  }
}

void mos6507_instruction_rra(uint16_t addr) {
  /*
  RRA - Rotate Right and Add with Carry

  M = M >> 1 | C, A = A + M + C
  Rotates memory right, then adds memory to accumulator with carry.
  */
  mos6507_instruction_ror(addr);
  mos6507_instruction_adc(mos6507_read(addr));
}

void mos6507_instruction_rti() {
  /*
  RTI - Return from Interrupt

  The RTI instruction is used at the end of an interrupt processing routine.
  It pulls the processor flags from the stack followed by the program counter.
  */
  processor_status = mos6507_stack_pull();
  program_counter = mos6507_stack_pull() << BYTE_SIZE;
  program_counter |= mos6507_stack_pull();
}

void mos6507_instruction_rts() {
  /*
  RTS - Return from Subroutine

  The RTS instruction is used at the end of a subroutine to return to the
  calling routine. It pulls the program counter (minus one) from the stack.
  */
  program_counter = mos6507_stack_pull() << BYTE_SIZE;
  program_counter |= mos6507_stack_pull();
}

void mos6507_instruction_sax(uint16_t addr) {
  /*
  SAX - Store Accumulator AND X Register

  M = A & X
  Stores the result of the accumulator AND the X register into memory.
  */
  mos6507_write(addr, accumulator & index_register_x);
}

void mos6507_instruction_sbc(uint8_t data) {
  /*
  SBC - Subtract with Carry

  A,Z,C,N = A-M-(1-C)
  This instruction subtracts the contents of a memory location to the
  accumulator together with the not of the carry bit. If overflow occurs the
  carry bit is clear, this enables multiple byte subtraction to be performed.
  */
  uint8_t subtract = data;

  if (!mos6507_flag_carry()) {
    ++subtract;
  }

  if (accumulator < subtract) {
    mos6507_flag_carry_unset();
  } else {
    mos6507_flag_carry_set();
  }

  accumulator -= subtract;
  mos6507_check_negative(accumulator);
  mos6507_check_zero(accumulator);
}

void mos6507_instruction_sbx(uint8_t data) {
  /*
  SBX - Subtract Memory from Accumulator AND X Register

  X = (A & X) - M
  */
  uint8_t val = (accumulator & index_register_x);
  if (val >= data)
    mos6507_flag_carry_set();
  else
    mos6507_flag_carry_unset();
  index_register_x = val - data;
  mos6507_check_negative(index_register_x);
  mos6507_check_zero(index_register_x);
}

void mos6507_instruction_sec() {
  /*
  SEC - Set Carry Flag

  C = 1
  Set the carry flag to one.
  */
  mos6507_flag_carry_set();
}

void mos6507_instruction_sed() {
  /*
  SED - Set Decimal Flag

  D = 1
  Set the decimal mode flag to one.
  */
  mos6507_flag_decimal_set();
}

void mos6507_instruction_sei() {
  /*
  SEI - Set Interrupt Disable

  I = 1
  Set the interrupt disable flag to one.
  */
  mos6507_flag_interrupt_set();
}

void mos6507_instruction_sha(uint16_t addr) {
  /*
  SHA - Store Accumulator AND X Register AND High Byte of Address

  M = A & X & H
  Stores the result of the accumulator AND the X register AND the high byte of
  the address into memory.
  */
  uint8_t high_byte = (addr >> 8);
  mos6507_write(addr, accumulator & index_register_x & high_byte);
}

void mos6507_instruction_slo(uint16_t addr) {
  /*
  SLO - Store ASL and ORA

  M = M << 1, C = bit 7 of M, A = A | M
  Shift memory left, then OR accumulator with memory.
  */
  mos6507_instruction_asl(addr);
  mos6507_instruction_ora(mos6507_read(addr));
}

void mos6507_instruction_sre(uint16_t addr) {
  /*
  SRE - Shift Right and Exclusive OR

  M = M >> 1, A = A ^ M
  Shifts memory right, then EORs accumulator with memory.
  */
  mos6507_instruction_lsr(addr);
  mos6507_instruction_eor(mos6507_read(addr));
}

void mos6507_instruction_sta(uint16_t addr) {
  /*
  STA - Store Accumulator

  M = A
  Stores the contents of the accumulator into memory.
  */
  mos6507_write(addr, accumulator);
}

void mos6507_instruction_stx(uint16_t addr) {
  /*
  STX - Store X Register

  M = X
  Stores the contents of the X register into memory.
  */
  mos6507_write(addr, index_register_x);
}

void mos6507_instruction_sty(uint16_t addr) {
  /*
  STY - Store Y Register

  M = Y
  Stores the contents of the Y register into memory.
  */
  mos6507_write(addr, index_register_y);
}

void mos6507_instruction_tas(uint16_t addr) {
  /*
  TAS - Transfer Accumulator AND X Register to Stack Pointer, and store in
  memory.

  S = A & X, M = S & H
  */
  stack_pointer = accumulator & index_register_x;
  mos6507_write(addr, stack_pointer & (addr >> 8));
}

void mos6507_instruction_tax() {
  /*
  TAX - Transfer Accumulator to X

  X = A
  Copies the current contents of the accumulator into the X register and sets
  the zero and negative flags as appropriate.
  */
  index_register_x = accumulator;
  mos6507_check_negative(index_register_x);
  mos6507_check_zero(index_register_x);
}

void mos6507_instruction_tay() {
  /*
  TAY - Transfer Accumulator to Y

  Y = A
  Copies the current contents of the accumulator into the Y register and sets
  the zero and negative flags as appropriate.
  */
  index_register_y = accumulator;
  mos6507_check_negative(index_register_y);
  mos6507_check_zero(index_register_y);
}

void mos6507_instruction_tsx() {
  /*
  TSX - Transfer Stack Pointer to X

  X = S
  Copies the current contents of the stack register into the X register and sets
  the zero and negative flags as appropriate.
  */
  index_register_x = stack_pointer;
  mos6507_check_negative(index_register_x);
  mos6507_check_zero(index_register_x);
}

void mos6507_instruction_txa() {
  /*
  TXA - Transfer X to Accumulator

  A = X
  Copies the current contents of the X register into the accumulator and sets
  the zero and negative flags as appropriate.
  */
  accumulator = index_register_x;
  mos6507_check_negative(accumulator);
  mos6507_check_zero(accumulator);
}

void mos6507_instruction_txs() {
  /*
  TXS - Transfer X to Stack Pointer

  S = X
  Copies the current contents of the X register into the stack register.
  */
  stack_pointer = index_register_x;
}

void mos6507_instruction_tya() {
  /*
  TYA - Transfer Y to Accumulator

  A = Y
  Copies the current contents of the Y register into the accumulator and sets
  the zero and negative flags as appropriate.
  */
  accumulator = index_register_y;
  mos6507_check_negative(accumulator);
  mos6507_check_zero(accumulator);
}

uint16_t mos6507_operand_2bytes() {
  uint8_t byte1;
  uint8_t byte2;

  mos6507_fetch();
  byte1 = instruction;
  mos6507_fetch();
  byte2 = instruction;
  return byte1 | (byte2 << BYTE_SIZE);
}

uint8_t mos6507_processor_status_flag(uint8_t flag_bit) {
  return (processor_status >> flag_bit) & 1;
}

void mos6507_processor_status_flag_set(uint8_t flag_bit) {
  processor_status |= (1 << flag_bit);
}

void mos6507_processor_status_flag_unset(uint8_t flag_bit) {
  processor_status &= ~(1 << flag_bit);
}

uint16_t mos6507_stack_pull() { return stack[++stack_pointer]; }

void mos6507_stack_push(uint16_t data) { stack[stack_pointer--] = data; }
