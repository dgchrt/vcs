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

extern unsigned char accumulator;
extern unsigned char index_register_x;
extern unsigned char index_register_y;
extern unsigned char instruction;
extern unsigned char processor_status;
extern unsigned short program_counter;
extern unsigned char registers[REGISTER_SIZE];
extern unsigned char running;
extern unsigned short stack[STACK_SIZE];
extern unsigned short stack_pointer;

unsigned short mos6507_addr_indirect_x();
unsigned short mos6507_addr_indirect_y();
signed char mos6507_addr_relative();
void mos6507_check_negative(unsigned char data);
void mos6507_check_overflow(unsigned short data);
void mos6507_check_zero(unsigned char data);
unsigned char mos6507_data_indirect_x();
unsigned char mos6507_data_indirect_y();
unsigned char mos6507_data_zero_page();
unsigned char mos6507_data_zero_page_x();
unsigned char mos6507_data_zero_page_y();
int mos6507_decode();
void mos6507_fetch();
void mos6507_flag_break_set();
unsigned char mos6507_flag_carry();
void mos6507_flag_carry_set();
void mos6507_flag_carry_unset();
void mos6507_flag_decimal_set();
void mos6507_flag_decimal_unset();
void mos6507_flag_interrupt_set();
void mos6507_flag_interrupt_unset();
unsigned char mos6507_flag_negative();
void mos6507_flag_negative_set();
void mos6507_flag_negative_unset();
unsigned char mos6507_flag_overflow();
void mos6507_flag_overflow_set();
void mos6507_flag_overflow_unset();
unsigned char mos6507_flag_zero();
void mos6507_flag_zero_set();
void mos6507_flag_zero_unset();

void mos6507_instruction_adc(unsigned char data);
void mos6507_instruction_alr(unsigned char data);
void mos6507_instruction_anc(unsigned char data);
void mos6507_instruction_and(unsigned char data);
void mos6507_instruction_ane(unsigned char data);
void mos6507_instruction_asl(unsigned short addr);
void mos6507_instruction_bcc(signed char addr);
void mos6507_instruction_bcs(signed char addr);
void mos6507_instruction_beq(signed char addr);
void mos6507_instruction_bit(unsigned char data);
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
void mos6507_instruction_cmp(unsigned char data);
void mos6507_instruction_cpx(unsigned char data);
void mos6507_instruction_cpy(unsigned char data);
void mos6507_instruction_dcp(unsigned short addr);
void mos6507_instruction_dec(unsigned short addr);
void mos6507_instruction_dex();
void mos6507_instruction_dey();
void mos6507_instruction_eor(unsigned char data);
void mos6507_instruction_inc(unsigned short addr);
void mos6507_instruction_inx();
void mos6507_instruction_iny();
void mos6507_instruction_isc(unsigned short addr);
void mos6507_instruction_jmp(unsigned short addr);
void mos6507_instruction_jsr(unsigned short addr);
void mos6507_instruction_las(unsigned char data);
void mos6507_instruction_lax(unsigned char data);
void mos6507_instruction_lda(unsigned char data);
void mos6507_instruction_ldx(unsigned char data);
void mos6507_instruction_ldy(unsigned char data);
void mos6507_instruction_lsr(unsigned short addr);
void mos6507_instruction_nop();
void mos6507_instruction_ora(unsigned char data);
void mos6507_instruction_pha();
void mos6507_instruction_php();
void mos6507_instruction_pla();
void mos6507_instruction_plp();
void mos6507_instruction_rla(unsigned short addr);
void mos6507_instruction_rol(unsigned short addr);
void mos6507_instruction_ror(unsigned short addr);
void mos6507_instruction_rra(unsigned short addr);
void mos6507_instruction_rti();
void mos6507_instruction_rts();
void mos6507_instruction_sax(unsigned short addr);
void mos6507_instruction_sbc(unsigned char data);
void mos6507_instruction_sbx(unsigned char data);
void mos6507_instruction_sec();
void mos6507_instruction_sed();
void mos6507_instruction_sei();
void mos6507_instruction_sha(unsigned short addr);
void mos6507_instruction_slo(unsigned short addr);
void mos6507_instruction_sre(unsigned short addr);
void mos6507_instruction_sta(unsigned short addr);
void mos6507_instruction_stx(unsigned short addr);
void mos6507_instruction_sty(unsigned short addr);
void mos6507_instruction_tas(unsigned short addr);
void mos6507_instruction_tax();
void mos6507_instruction_tay();
void mos6507_instruction_tsx();
void mos6507_instruction_txa();
void mos6507_instruction_txs();
void mos6507_instruction_tya();

unsigned short mos6507_operand_2bytes();
unsigned char mos6507_processor_status_flag(unsigned char flag_bit);
void mos6507_processor_status_flag_set(unsigned char flag_bit);
void mos6507_processor_status_flag_unset(unsigned char flag_bit);
unsigned short mos6507_stack_pull();
void mos6507_stack_push(unsigned short data);
