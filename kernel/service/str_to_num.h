#ifndef _STR_TO_NUM_H
#define _STR_TO_NUM_H

#include <stdint.h>
#include <stddef.h>

int32_t str_to_int32(const char* str);
int64_t str_to_int64(const char* str);
int32_t hstr_to_int32(const char* str);
int64_t hstr_to_int64(const char* str);
uint32_t str_to_uint32(const char* str);
uint64_t str_to_uint64(const char* str);
uint32_t hstr_to_uint32(const char* str);
uint64_t hstr_to_uint64(const char* str);

int64_t str_to_num(const char* str);
uint64_t str_to_unum(const char* str);

bool is_hex_prefix(const char* str);
const char* skip_whitespace(const char* str);

const char* skip_whitespace(const char* str) {
	while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
		str++;
	}
	return str++;
}
int32_t str_to_int32(const char* str) {
    if (str == NULL)return 0;
    str = skip_whitespace(str);

    int32_t result = 0;
    int8_t sign = 1;

    if (*str == '-') {
        sign = -1;
        str++;
    }
    else if (*str == '+') {
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        if (result > INT32_MAX / 10) {
            return (sign == 1) ? INT32_MAX : INT32_MIN;
        }
        if (result == INT32_MAX / 10) {
            if (sign == 1 && (*str - '0') > INT32_MAX % 10) {
                return INT32_MAX;
            }
            if (sign == -1 && (*str - '0') > -(INT32_MIN % 10)) {
                return INT32_MIN;
            }
        }

        result = result * 10 + (*str - '0');
        str++;
    }

    return result * sign;
}
uint32_t str_to_uint32(const char* str) {
    if (str == NULL) return 0;

    str = skip_whitespace(str);

    uint32_t result = 0;

    if (*str == '+') {
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        if (result > UINT32_MAX / 10) {
            return UINT32_MAX;
        }
        if (result == UINT32_MAX / 10 && (*str - '0') > UINT32_MAX % 10) {
            return UINT32_MAX;
        }

        result = result * 10 + (*str - '0');
        str++;
    }

    return result;
}
int64_t str_to_int64(const char* str) {
    if (str == NULL) return 0;

    str = skip_whitespace(str);

    int64_t result = 0;
    int8_t sign = 1;

    if (*str == '-') {
        sign = -1;
        str++;
    }
    else if (*str == '+') {
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        if (result > INT64_MAX / 10) {
            return (sign == 1) ? INT64_MAX : INT64_MIN;
        }
        if (result == INT64_MAX / 10) {
            if (sign == 1 && (*str - '0') > INT64_MAX % 10) {
                return INT64_MAX;
            }
            if (sign == -1 && (*str - '0') > -(INT64_MIN % 10)) {
                return INT64_MIN;
            }
        }

        result = result * 10 + (*str - '0');
        str++;
    }

    return result * sign;
}
uint64_t str_to_uint64(const char* str) {
    if (str == NULL) return 0;

    str = skip_whitespace(str);

    uint64_t result = 0;

    if (*str == '+') {
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        if (result > UINT64_MAX / 10) {
            return UINT64_MAX;
        }
        if (result == UINT64_MAX / 10 && (*str - '0') > UINT64_MAX % 10) {
            return UINT64_MAX;
        }

        result = result * 10 + (*str - '0');
        str++;
    }

    return result;
}
bool is_hex_prefix(const char* str) {
    return (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'));
}
static int8_t hex_char_to_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
int32_t hstr_to_int32(const char* str) {
    if (str == NULL) return 0;

    str = skip_whitespace(str);

    if (is_hex_prefix(str)) {
        str += 2;
    }

    int32_t result = 0;
    int8_t sign = 1;

    if (*str == '-') {
        sign = -1;
        str++;
    }
    else if (*str == '+') {
        str++;
    }

    while (*str) {
        int8_t value = hex_char_to_value(*str);
        if (value < 0) break;

        if (result > INT32_MAX / 16) {
            return (sign == 1) ? INT32_MAX : INT32_MIN;
        }
        if (result == INT32_MAX / 16) {
            if (sign == 1 && value > INT32_MAX % 16) {
                return INT32_MAX;
            }
            if (sign == -1 && value > -(INT32_MIN % 16)) {
                return INT32_MIN;
            }
        }

        result = result * 16 + value;
        str++;
    }

    return result * sign;
}
uint32_t hstr_to_uint32(const char* str) {
    if (str == NULL) return 0;

    str = skip_whitespace(str);

    if (is_hex_prefix(str)) {
        str += 2;
    }

    uint32_t result = 0;

    while (*str) {
        int8_t value = hex_char_to_value(*str);
        if (value < 0) break;

        if (result > UINT32_MAX / 16) {
            return UINT32_MAX;
        }
        if (result == UINT32_MAX / 16 && value > UINT32_MAX % 16) {
            return UINT32_MAX;
        }

        result = result * 16 + value;
        str++;
    }

    return result;
}
int64_t hstr_to_int64(const char* str) {
    if (str == NULL) return 0;

    str = skip_whitespace(str);

    if (is_hex_prefix(str)) {
        str += 2;
    }

    int64_t result = 0;
    int8_t sign = 1;

    if (*str == '-') {
        sign = -1;
        str++;
    }
    else if (*str == '+') {
        str++;
    }

    while (*str) {
        int8_t value = hex_char_to_value(*str);
        if (value < 0) break;

        if (result > INT64_MAX / 16) {
            return (sign == 1) ? INT64_MAX : INT64_MIN;
        }
        if (result == INT64_MAX / 16) {
            if (sign == 1 && value > INT64_MAX % 16) {
                return INT64_MAX;
            }
            if (sign == -1 && value > -(INT64_MIN % 16)) {
                return INT64_MIN;
            }
        }

        result = result * 16 + value;
        str++;
    }

    return result * sign;
}
uint64_t hstr_to_uint64(const char* str) {
    if (str == NULL) return 0;

    str = skip_whitespace(str);

    if (is_hex_prefix(str)) {
        str += 2;
    }

    uint64_t result = 0;

    while (*str) {
        int8_t value = hex_char_to_value(*str);
        if (value < 0) break;

        if (result > UINT64_MAX / 16) {
            return UINT64_MAX;
        }
        if (result == UINT64_MAX / 16 && value > UINT64_MAX % 16) {
            return UINT64_MAX;
        }

        result = result * 16 + value;
        str++;
    }

    return result;
}
int64_t str_to_num(const char* str) {
    if (str == NULL) return 0;

    str = skip_whitespace(str);

    if (is_hex_prefix(str)) {
        return hstr_to_int64(str);
    }

    return str_to_int64(str);
}
uint64_t str_to_unum(const char* str) {
    if (str == NULL) return 0;

    str = skip_whitespace(str);

    if (is_hex_prefix(str)) {
        return hstr_to_uint64(str);
    }

    return str_to_uint64(str);
}
#endif
