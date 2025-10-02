#ifndef _ACPI_H_
#define _ACPI_H_

#include <kernel.h>
#include <stdint.h>
#include <driver/io.h>

#define ACPI_PM1_CTL	0x804
#define ACPI_PM1_STS	0x800
#define SLP_EN			0x2000
#define SLP_TYP			0x1C00

int check_acpi_support() {
	return 1;
}
void acpi_shutdown() {
	if (!check_acpi_support()) {
		printk(" * ACPI not supported.\n");
		return;
	}

	asm volatile("cli");

	outw(SLP_EN | (5 << 10), ACPI_PM1_CTL);

	while (1) {
		asm volatile("hlt");
	}
}

#endif
