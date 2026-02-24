#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>
#include <driver/io.h>
#include <kernel.h>
#include <speaker.h>
#include <mm/pcb.h>

// PIT �� I/O �˿�
#define PIT_CHANNEL0_DATA 0x40
#define PIT_CHANNEL1_DATA 0x41
#define PIT_CHANNEL2_DATA 0x42
#define PIT_COMMAND_PORT  0x43

// PIT �����ָ�ʽ
#define PIT_CMD_CHANNEL0     0x00 // ѡ��ͨ�� 0
#define PIT_CMD_LOHI         0x30 // ��д���ֽڣ���д���ֽ� (����ģʽ)
#define PIT_CMD_MODE3        0x06 // ����������ģʽ����õĶ�ʱģʽ��
#define PIT_CMD_BINARY       0x00 // 16λ������ģʽ

// ���������
#define PIT_CMD_INIT (PIT_CMD_CHANNEL0 | PIT_CMD_LOHI | PIT_CMD_MODE3 | PIT_CMD_BINARY)

// ������������
#define PIT_BASE_FREQ 1193182 // ��ʱ������Ƶ��

#define FREQ_T0 1 // 100 Hz
int intcnt = 0;
int cnt2 = 0;
int melody[3] = { 440, 660, 880 };

extern void timer_hd(void);
void timer_handler(void) {
	schedule();
	outb(0x20, 0x20);
}

void pit_init(void) {
	uint16_t reload_value = PIT_BASE_FREQ / FREQ_T0;

	idt_set_gate(0x20, (uint32_t)schedule, 0x08, 0x8e);
	idt_flush();

	outb(PIT_CMD_INIT, PIT_COMMAND_PORT);
	outb(reload_value & 0xFF, PIT_CHANNEL0_DATA);
	outb((reload_value >> 8) & 0xFF, PIT_CHANNEL0_DATA);

	IRQ_clear_mask(0);
	printk(" - PIT for %d Hz, reload %x\n", FREQ_T0, reload_value);
}

#endif