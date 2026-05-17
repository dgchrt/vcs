#include "hal.h"
#include <stdio.h>
#include <stdlib.h>

#define BYTE_SIZE 8
#define KEY_ESC 27
#define VECTOR_RESET 0x1ffc
#define VECTOR_IRQ 0x1ffe
/*
0x0000 - 0x007f: TIA
0x0080 - 0x00ff: RAM (128 bytes)
0x0200 - 0x02ff: RIOT
0x1000 - 0x1fff: ROM
*/
#define MILLISECONDS 1000
#define REGISTER_SIZE 0x10
#define ROM_ADDRESS 0x1000
#define STACK_SIZE 0x100
#define STATUS_BIT_BREAK 4
#define STATUS_BIT_CARRY 0
#define STATUS_BIT_DECIMAL 3
#define STATUS_BIT_INTERRUPT 2
#define STATUS_BIT_NEGATIVE 7
#define STATUS_BIT_OVERFLOW 6
#define STATUS_BIT_ZERO 1

extern uint8_t accumulator;
extern uint8_t index_register_x;
extern uint8_t index_register_y;
extern uint8_t instruction;
extern uint8_t processor_status;
extern uint16_t program_counter;
extern uint8_t registers[REGISTER_SIZE];
extern uint8_t running;
extern uint16_t stack[STACK_SIZE];
extern uint16_t stack_pointer;

uint16_t mos6507_addr_indirect_x();
uint16_t mos6507_addr_indirect_y();
signed char mos6507_addr_relative();
void mos6507_check_negative(uint8_t data);
void mos6507_check_overflow(uint16_t data);
void mos6507_check_zero(uint8_t data);
uint8_t mos6507_data_indirect_x();
uint8_t mos6507_data_indirect_y();
uint8_t mos6507_data_zero_page();
uint8_t mos6507_data_zero_page_x();
uint8_t mos6507_data_zero_page_y();
int mos6507_decode();
void mos6507_fetch();
void mos6507_flag_break_set();
uint8_t mos6507_flag_carry();
void mos6507_flag_carry_set();
void mos6507_flag_carry_unset();
void mos6507_flag_decimal_set();
void mos6507_flag_decimal_unset();
void mos6507_flag_interrupt_set();
void mos6507_flag_interrupt_unset();
uint8_t mos6507_flag_negative();
void mos6507_flag_negative_set();
void mos6507_flag_negative_unset();
uint8_t mos6507_flag_overflow();
void mos6507_flag_overflow_set();
void mos6507_flag_overflow_unset();
uint8_t mos6507_flag_zero();
void mos6507_flag_zero_set();
void mos6507_flag_zero_unset();

void mos6507_instruction_adc(uint8_t data);
void mos6507_instruction_alr(uint8_t data);
void mos6507_instruction_anc(uint8_t data);
void mos6507_instruction_and(uint8_t data);
void mos6507_instruction_ane(uint8_t data);
void mos6507_instruction_asl(uint16_t addr);
void mos6507_instruction_bcc(signed char addr);
void mos6507_instruction_bcs(signed char addr);
void mos6507_instruction_beq(signed char addr);
void mos6507_instruction_bit(uint8_t data);
void mos6507_instruction_bmi(signed char addr);
void mos6507_instruction_bne(signed char addr);
void mos6507_instruction_bpl(signed char addr);
void mos6507_instruction_brk();
void mos6507_instruction_bvc(signed char addr);
void mos6507_instruction_bvs(signed char addr);
void mos6507_instruction_clc();
void mos6507_instruction_cld();
void mos6507_instruction_cli();
void mos6507_instruction_clv();
void mos6507_instruction_cmp(uint8_t data);
void mos6507_instruction_cpx(uint8_t data);
void mos6507_instruction_cpy(uint8_t data);
void mos6507_instruction_dcp(uint16_t addr);
void mos6507_instruction_dec(uint16_t addr);
void mos6507_instruction_dex();
void mos6507_instruction_dey();
void mos6507_instruction_eor(uint8_t data);
void mos6507_instruction_inc(uint16_t addr);
void mos6507_instruction_inx();
void mos6507_instruction_iny();
void mos6507_instruction_isc(uint16_t addr);
void mos6507_instruction_jmp(uint16_t addr);
void mos6507_instruction_jsr(uint16_t addr);
void mos6507_instruction_las(uint8_t data);
void mos6507_instruction_lax(uint8_t data);
void mos6507_instruction_lda(uint8_t data);
void mos6507_instruction_ldx(uint8_t data);
void mos6507_instruction_ldy(uint8_t data);
void mos6507_instruction_lsr(uint16_t addr);
void mos6507_instruction_nop();
void mos6507_instruction_ora(uint8_t data);
void mos6507_instruction_pha();
void mos6507_instruction_php();
void mos6507_instruction_pla();
void mos6507_instruction_plp();
void mos6507_instruction_rla(uint16_t addr);
void mos6507_instruction_rol(uint16_t addr);
void mos6507_instruction_ror(uint16_t addr);
void mos6507_instruction_rra(uint16_t addr);
void mos6507_instruction_rti();
void mos6507_instruction_rts();
void mos6507_instruction_sax(uint16_t addr);
void mos6507_instruction_sbc(uint8_t data);
void mos6507_instruction_sbx(uint8_t data);
void mos6507_instruction_sec();
void mos6507_instruction_sed();
void mos6507_instruction_sei();
void mos6507_instruction_sha(uint16_t addr);
void mos6507_instruction_slo(uint16_t addr);
void mos6507_instruction_sre(uint16_t addr);
void mos6507_instruction_sta(uint16_t addr);
void mos6507_instruction_stx(uint16_t addr);
void mos6507_instruction_sty(uint16_t addr);
void mos6507_instruction_tas(uint16_t addr);
void mos6507_instruction_tax();
void mos6507_instruction_tay();
void mos6507_instruction_tsx();
void mos6507_instruction_txa();
void mos6507_instruction_txs();
void mos6507_instruction_tya();

uint16_t mos6507_operand_2bytes();
uint8_t mos6507_processor_status_flag(uint8_t flag_bit);
void mos6507_processor_status_flag_set(uint8_t flag_bit);
void mos6507_processor_status_flag_unset(uint8_t flag_bit);
uint16_t mos6507_stack_pull();
void mos6507_stack_push(uint16_t data);
