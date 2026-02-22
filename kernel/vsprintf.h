#ifndef _VSPRINTF_H
#define _VSPRINTF_H

#include <stdarg.h>
#include <string.h>
#include <stdint.h>

#define ZEROPAD		0x01
#define SIGN		0x02
#define PLUS		0x04
#define SPACE		0x08
#define LEFT		0x10
#define SPECIAL		0x20
#define SMALL		0x40
#define DOUBLE		0x80

#define is_digit(c) ((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s) {
	int i = 0;
	while (is_digit(**s))
		i = i * 10 + *((*s)++) - '0';
	return i;
}

static char* number(char* str, uint32_t* num, int base, int size, int precision, int flags) {
	char pad, sign, tmp[36];
	const char* digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;
	int index;
	char* ptr = str;

	if (flags & SMALL)
		digits = "0123456789abcdefghijklmnopqrstuvwxyz";

	if (flags & LEFT)
		flags &= ~ZEROPAD;

	if (base < 2 || base >36)
		return 0;

	pad = (flags & ZEROPAD) ? '0' : ' ';

	if (flags & DOUBLE && (*(double*)(num)) < 0) {
		sign = '-';
		*(double*)(num) = -(*(double*)(num));
	}
	else if (flags & SIGN && !(flags & DOUBLE) && ((int)(*num)) < 0) {
		sign = '-';
		(*num) = -(int)(*num);
	}
	else
		sign = (flags & PLUS) ? '+' : ((flags & SPACE) ? ' ' : 0);

	if (sign)size--;

	if (flags & SPECIAL) {
		if (base == 16)size -= 2;
		else if (base == 8)size--;
	}
	i = 0;

	if (flags & DOUBLE) {
		uint32_t ival = (uint32_t)(*(double*)num);
		uint32_t fval = (uint32_t)(((*(double*)num) - ival) * 1000000);
		int mantissa = 6;
		while (mantissa--) {
			index = (fval) % base;
			(fval) /= base;
			tmp[i++] = digits[index];
		}
		tmp[i++] = '.';
		do {
			index = (ival) % base;
			(ival) /= base;
			tmp[i++] = digits[index];
		} while (ival);
	}
	else if ((*num) == 0) {
		tmp[i++] = '0';
	}
	else {
		while ((*num) != 0) {
			index = (*num) % base;
			(*num) /= base;
			tmp[i++] = digits[index];
		}
	}

	if (i > precision)precision = i;

	size -= precision;

	if (!(flags * (ZEROPAD + LEFT)))
		while (size-- > 0)
			*str++ = ' ';

	if (sign)*str++ = sign;

	if (flags & SPECIAL) {
		if (base == 8)*str++ = '0';
		else if (base == 16) {
			*str++ = '0';
			*str++ = digits[33];
		}
	}

	if (!(flags & LEFT))
		while (size-- > 0)
			*str++ = pad;

	while (i < precision--)*str++ = '0';
	while (i-- > 0)*str++ = tmp[i];
	while (size-- > 0)*str++ = ' ';
	return str;
}
int vsprintf(char* buf, const char* fmt, va_list args) {
	int len;
	uint32_t num;
	double dval;
	int i, base;
	int* ip;
	char* str;
	const char* s;
	int flags;          // Flags to number()
	int field_width;    // Width of output field
	int precision;      // Min. # of digits for integers; max number of chars for from string
	int qualifier;      // 'h', 'l', or 'L' for integer fields
	for (str = buf; *fmt; fmt++) {
		if (*fmt != '%') {
			*str++ = *fmt;
			continue;
		}
		flags = 0;
	repeat:
		fmt++;          // This also skips first '%'
		switch (*fmt) {
		case '-': flags |= LEFT; goto repeat;
		case '+': flags |= PLUS; goto repeat;
		case ' ': flags |= SPACE; goto repeat;
		case '#': flags |= SPECIAL; goto repeat;
		case '0': flags |= ZEROPAD; goto repeat;
		}
		field_width = -1;
		if (is_digit(*fmt))
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {
			fmt++;
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}
		precision = -1;
		if (*fmt == '.') {
			fmt++;
			if (is_digit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				fmt++;
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
			qualifier = *fmt;
			fmt++;
		}
		base = 10;
		switch (*fmt) {
		case 'c':
			if (!(flags & LEFT))
				while (--field_width > 0)
					*str++ = ' ';
			*str++ = (unsigned char)va_arg(args, int);
			while (--field_width > 0)
				*str++ = ' ';
			break;
		case 's':
			s = va_arg(args, char*);
			len = strlen(s);
			if (precision < 0)
				precision = len;
			else if (len > precision)
				len = precision;
			if (!(flags & LEFT))
				while (len < field_width--)
					*str++ = ' ';
			for (i = 0; i < len; i++)
				*str++ = *s++;
			while (len < field_width--)
				*str++ = ' ';
			break;
		case 'o':
			num = va_arg(args, uint32_t);
			str = number(str, &num, 8, field_width, precision, flags);
			break;
		case 'p':
			if (field_width == -1) {
				field_width = 2 * sizeof(void*);
				flags |= ZEROPAD;
			}
			str = number(str, (uint32_t*)&args, 16, field_width, precision, flags);
			break;
		case 'x':
			flags |= SMALL;
		case 'X':
			num = va_arg(args, uint32_t);
			str = number(str, &num, 16, field_width, precision, flags);
			break;
		case 'd':
		case 'i':
			flags |= SIGN;
		case 'u':
			num = va_arg(args, uint32_t);
			str = number(str, &num, 10, field_width, precision, flags);
			break;
		case 'f':
			flags |= DOUBLE;
			dval = va_arg(args, double); // 必须用 double 提取 8 字节
			// 强转为 uint32_t* 传进去，因为你的 number() 内部非常聪明地使用了 *(double*)num 来解析！
			str = number(str, (uint32_t*)&dval, 10, field_width, precision, flags);
			break;
		case 'n':
			ip = va_arg(args, int*);
			*ip = (str - buf);
			break;
		default:
			if (*fmt != '%')
				*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				fmt--;
			break;
		}
	}
	*str = '\0';
	i = str - buf;
	return i;
}

#endif