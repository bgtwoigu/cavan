#pragma once

/*
 * Author: Fuang.Cao
 * Email: cavan.cfa@gmail.com
 * Date: Mon Nov 26 16:08:05 CST 2012
 */

#include <cavan.h>

#define MIN(a, b) \
	((a) < (b) ? (a) : (b))

#define MAX(a, b) \
	((a) > (b) ? (a) : (b))

#define ABS(a) \
	((a) < 0 ? -(a) : (a))

#define DIV_CEIL(a, b) \
	(((a) + (b) - 1) / (b))

#define RIGHT_SHIFT_CEIL(a, shift) \
	((a) < 1 ? 0 : (((a) - 1) >> (shift)) + 1)

#define ROR(value, bits) \
	((value) >> (bits) | (value) << ((sizeof(value) << 3) - (bits)))

#define ROL(value, bits) \
	((value) << (bits) | (value) >> ((sizeof(value) << 3) - (bits)))

#define FFS(value) \
	({ \
		int __offset; \
		switch (sizeof(value)) { \
		case 1: \
			__offset = math_find_first_non_zero_bit8(value); \
			break; \
		case 2: \
			__offset = math_find_first_non_zero_bit16(value); \
			break; \
		case 4: \
			__offset = math_find_first_non_zero_bit32(value); \
			break; \
		case 8: \
			__offset = math_find_first_non_zero_bit64(value); \
			break; \
		default: \
			__offset = -EFAULT; \
		} \
		__offset; \
	})

#define FLS(value) \
	({ \
		int __offset; \
		switch (sizeof(value)) { \
		case 1: \
			__offset = math_find_last_non_zero_bit8(value); \
			break; \
		case 2: \
			__offset = math_find_last_non_zero_bit16(value); \
			break; \
		case 4: \
			__offset = math_find_last_non_zero_bit32(value); \
			break; \
		case 8: \
			__offset = math_find_last_non_zero_bit64(value); \
			break; \
		default: \
			__offset = -EFAULT; \
		} \
		__offset; \
	})

byte *math_memory_shrink(const byte *mem, size_t size);
void math_memory_exchange(const byte *mem, byte *res, size_t size);
void math_memory_copy(byte *dest, size_t dest_size, const byte *src, size_t src_size);
void math_memory_complement(const byte *mem, size_t mem_size, byte *res, size_t res_size);
char *math_text2memory(const char *text, byte *mem, size_t mem_size, int base);
char *math_memory2text(const byte *mem, size_t mem_size, char *text, size_t text_size, int base, char fill, size_t size);
char *math_memory_remain2text(const byte *left, size_t lsize, const byte *right, size_t rsize, char *text, size_t tsize, int base, char fill, size_t size);
void math_memory_show(const char *prompt, const byte *mem, size_t mem_size, int base);

void math_memory_shift_left_byte(const byte *mem, size_t mem_size, size_t shift, byte *res, size_t res_size);
void math_memory_shift_left_bit(const byte *mem, size_t size, size_t shift, byte *res);
void math_memory_shift_left(const byte *mem, size_t mem_size, size_t shift, byte *res, size_t res_size);

void math_memory_ring_shift_left_byte(const byte *mem, size_t mem_size, size_t shift, byte *res, size_t res_size);
void math_memory_ring_shift_left_bit(const byte *mem, size_t size, size_t shift, byte *res);
void math_memory_ring_shift_left(const byte *mem, size_t mem_size, size_t shift, byte *res, size_t res_size);

void math_memory_shift_right_byte(const byte *mem, size_t mem_size, size_t shift, byte *res, size_t res_size);
void math_memory_shift_right_bit(const byte *mem, size_t size, size_t shift, byte *res);
void math_memory_shift_right(const byte *mem, size_t mem_size, size_t shift, byte *res, size_t res_size);

void math_memory_ring_shift_right_byte(const byte *mem, size_t mem_size, size_t shift, byte *res, size_t res_size);
void math_memory_ring_shift_right_bit(const byte *mem, size_t size, size_t shift, byte *res);
void math_memory_ring_shift_right(const byte *mem, size_t mem_size, size_t shift, byte *res, size_t res_size);

void math_memory_and(const byte *left, const byte *right, byte *res, size_t size);
void math_memory_or(const byte *left, const byte *right, byte *res, size_t size);
void math_memory_not(const byte *mem, byte *res, size_t size);
void math_memory_xor(const byte *left, const byte *right, byte *res, size_t size);

byte math_memory_add_single(const byte *mem, size_t mem_size, byte value, byte *res, size_t res_size);
byte math_memory_add(const byte *left, size_t lsize, const byte *right, size_t rsize, byte *res, size_t res_size);

int math_memory_cmp_base(const byte *left_last, const byte *right, const byte *right_last);
int math_memory_cmp(const byte *left, size_t lsize, const byte *right, size_t rsize);
byte math_byte_sub_carry(byte left, byte right, byte carry, byte *res);
byte math_memory_sub_single(const byte *mem, size_t mem_size, byte value, byte carry, byte *res, size_t res_size);
byte math_memory_sub_single2(byte value, byte carry, const byte *mem, size_t mem_size, byte *res, size_t res_size);
byte math_memory_sub(const byte *left, size_t lsize, const byte *right, size_t rsize, byte *res, size_t res_size);

byte math_memory_mul_single(const byte *mem, size_t mem_size, byte value, byte *res, size_t res_size);
byte math_memory_mul_unsigned(const byte *left, size_t lsize, const byte *right, size_t rsize, byte *res, size_t res_size);
byte math_memory_mul(const byte *left, size_t lsize, const byte *right, size_t rsize, byte *res, size_t res_size);

byte math_memory_div_single(const byte *mem, size_t mem_size, byte value, byte *res, size_t *res_size);
byte math_memory_div_once(byte *left, size_t lsize, const byte *right, size_t rsize, byte *res, size_t res_size);
size_t math_memory_div_unsigned(byte *left, size_t lsize, const byte *right, size_t rsize, byte *res, size_t res_size);
size_t math_memory_div(byte *left, size_t lsize, const byte *right, size_t rsize, byte *res, size_t res_size);
size_t math_memory_div2(byte *left, size_t lsize, const byte *right, size_t rsize, byte *res, size_t res_size, int base);

int math_memory_calculator(const char *formula, byte *res, size_t res_size, int base, char fill, int size);

int math_find_first_non_zero_bit8(u8 value);
int math_find_first_non_zero_bit16(u16 value);
int math_find_first_non_zero_bit32(u32 value);
int math_find_first_non_zero_bit64(u64 value);

int math_find_last_non_zero_bit8(u8 value);
int math_find_last_non_zero_bit16(u16 value);
int math_find_last_non_zero_bit32(u32 value);
int math_find_last_non_zero_bit64(u64 value);

ulong math_get_greatest_common_divisor_single(ulong a, ulong b);
ulong math_get_greatest_common_divisor(const ulong *data, size_t count);
ulong math_get_lowest_common_multiple_single(ulong a, ulong b);
ulong math_get_lowest_common_multiple(const ulong *data, size_t count);

static inline bool math_memory_is_negative(const byte *mem, size_t size)
{
	return mem[size - 1] & (1 << 7);
}

#if __WORDSIZE > 32
static inline int math_get_value_shift(u64 value)
{
	return math_find_first_non_zero_bit64(value);
}
#elif __WORDSIZE > 16
static inline int math_get_value_shift(u32 value)
{
	return math_find_first_non_zero_bit32(value);
}
#elif __WORDSIZE > 8
static inline int math_get_value_shift(u16 value)
{
	return math_find_first_non_zero_bit16(value);
}
#else
static inline int math_get_value_shift(u8 value)
{
	return math_find_first_non_zero_bit8(value);
}
#endif
