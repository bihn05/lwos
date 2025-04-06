#ifndef _ASSERT_H
#define _ASSERT_H

#include <pristdio.h>
#include <type.h>
#include <stdarg.h>
// 强制阻塞
static void spin(char* name)
{
	printk("spinning in %s ...\n", name);
	while (true)
		;
}
static void block(char* name) {
	outstr("Block in ");
	outstr(name);
	outstr(" ...\n");
	while (1);
}
void assertion_failure(char* exp, char* file, char* base, int line) {
	outstr("\n-->assert(");
	outstr(exp);
	outstr(") failed.\n-->file: ");
	outstr(file);
	outstr("\n-->base: ");
	outstr(base);
	outstr("\n-->line: 0x");
	iouthex32(line);
	outstr("\n");
	block("ASSERTION_FAILURE()");
}

#define assert(exp); if(exp);else assertion_failure(#exp, __FILE__, __BASE_FILE__, __LINE__);

void panic()
{

	spin("panic()");

	// 不可能走到这里，否则出错；
	asm volatile("ud2");
}

#endif