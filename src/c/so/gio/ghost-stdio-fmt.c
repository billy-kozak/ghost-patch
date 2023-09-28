/**************************************
* Copyright (C) 2023  Billy Kozak                                             *
*                                                                             *
* This file is part of the ghost-patch program                                *
*                                                                             *
* This program is free software: you can redistribute it and/or modify        *
* it under the terms of the GNU Lesser General Public License as published by *
* the Free Software Foundation, either version 3 of the License, or           *
* (at your option) any later version.                                         *
*                                                                             *
* This program is distributed in the hope that it will be useful,             *
* but WITHOUT ANY WARRANTY; without even the implied warranty of              *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
* GNU Lesser General Public License for more details.                         *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this program.  If not, see <http://www.gnu.org/licenses/>.       *
******************************************************************************/
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include "ghost-stdio.h"
#include "ghost-stdio-internal.h"

#include <utl/math-utl.h>
#include <gmalloc/ghost-malloc.h>
#include <secret-heap.h>
#include <gio/musl-fmt-double.h>
#include <circ_buffer.h>

#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <wchar.h>
#include <limits.h>
/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
enum length_mod {
	LMOD_NONE,
	LMOD_H,
	LMOD_HH,
	LMOD_L,
	LMOD_LL,
	LMOD_J,
	LMOD_Z,
	LMOD_T
};

enum conv {
	CONV_INT,
	CONV_UINT,
	CONV_UINT_O,
	CONV_UINT_X,
	CONV_UINT_XX,
	CONV_FLOAT_E,
	CONV_FLOAT_EE,
	CONV_FLOAT,
	CONV_FLOAT_G,
	CONV_FLOAT_GG,
	CONV_FLOAT_X,
	CONV_FLOAT_XX,
	CONV_CHAR,
	CONV_STR,
	CONV_POINT,
	CONV_CCOUNT,
	CONV_ERRNO,
	CONV_PERCENT,
	CONV_MOD
};

union arg_val {
	int i;
	unsigned u;
	long int li;
	long unsigned lu;
	long long int lli;
	long long unsigned llu;

	intmax_t im;
	uintmax_t um;

	size_t st;
	ssize_t sst;

	ptrdiff_t pd;

	double d;
	long double ld;

	void *p;
};

struct fmt_arg {
	int pos;
	int flags;
	int width;
	int prec;
	int aidx;
	union arg_val val;
	enum length_mod mod;
	enum conv conv;
};

struct fmt_arg_list {
	size_t len;
	size_t size;

	struct fmt_arg args[];
};

struct output_str {
	char *str;
	size_t len;

	int i;
};

struct output_file {
	struct ghost_file *f;
	int i;
};
/******************************************************************************
*                                  CONSTANTS                                  *
******************************************************************************/
#define FLAG_ALT  1
#define FLAG_ZPAD  2
#define FLAG_LADJ 4
#define FLAG_SSPC 8
#define FLAG_PSIN 16

#define FMT_ARG_LIST_INIT_LEN 8

#define PREC_UNDEF INT_MIN

#define DYN_STR_INIT_LEN 1024
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static int parse_flags(const char *restrict fmt, size_t pos, int *flags)
{
	*flags = 0;
	int idx = pos;

	while(fmt[idx] != '\0') {
		switch(fmt[idx]) {
		case '#':
			*flags |= FLAG_ALT;
			break;
		case '0':
			*flags |= FLAG_ZPAD;
			break;
		case '-':
			*flags |= FLAG_LADJ;
			break;
		case ' ':
			*flags |= FLAG_SSPC;
			break;
		case '+':
			*flags |= FLAG_PSIN;
			break;
		default:
			return idx - pos;
		}
		idx += 1;
	}

	return idx - pos;
}
/*****************************************************************************/
static int parse_pos(const char *restrict fmt, size_t pos, int *posn)
{
	char *endptr;
	int temp = strtoul(fmt + pos, &endptr, 10);

	if(endptr == fmt + pos) {
		*posn += 1;
		return 0;
	}
	if(endptr[0] != '$') {
		*posn += 1;
		return 0;
	}

	*posn = temp;
	return endptr - (fmt + pos);
}
/*****************************************************************************/
static int parse_awidth(const char *restrict fmt, size_t pos, int *posn)
{
	if(fmt[pos] != '*') {
		return 0;
	}

	char *endptr;
	int temp = strtoul(fmt + pos + 1, &endptr, 10);

	if(endptr == fmt + pos + 1) {
		*posn += 1;
		return 1;
	} else {
		*posn = temp;
		return endptr - (fmt + pos + 1);
	}
}
/*****************************************************************************/
static int parse_fwidth(const char *restrict fmt, size_t pos, int *width)
{
	int temp = -1;
	char *endptr;

	temp = strtoul(fmt + pos, &endptr, 10);

	if(endptr == fmt + pos) {
		return 0;
	} else {
		*width = temp;
		return endptr - (fmt + pos);
	}
}
/*****************************************************************************/
static int parse_aprec(const char *restrict fmt, size_t pos, int *posn)
{
	if(fmt[pos] != '.' || fmt[pos + 1] != '*') {
		return 0;
	}

	int consumed = parse_awidth(fmt, pos + 1, posn) + 1;

	assert(consumed > 1);

	return consumed;
}
/*****************************************************************************/
static int parse_fprec(const char *restrict fmt, size_t pos, int *prec)
{
	if(fmt[pos] != '.') {
		*prec = PREC_UNDEF;
		return 0;
	}

	int consumed = parse_fwidth(fmt, pos + 1, prec) + 1;

	if(consumed == 1) {
		*prec = 0;
	}

	return consumed;
}
/*****************************************************************************/
static int parse_length_mod(
	const char *restrict fmt,
	size_t pos,
	enum length_mod *mod
) {
	switch(fmt[pos]) {
	case 'h':
		if(fmt[pos + 1] == 'h') {
			*mod = LMOD_HH;
			return 2;
		} else {
			*mod = LMOD_H;
			return 1;
		}
	case 'l':
		if(fmt[pos + 1] == 'l') {
			*mod = LMOD_LL;
			return 2;
		} else {
			*mod = LMOD_L;
			return 1;
		}
	case 'q':
		/* fallthrough */
	case 'L':
		*mod = LMOD_LL;
		return 1;
	case 'j':
		*mod = LMOD_J;
		return 1;
	case 'z':
		/* fallthrough */
	case 'Z':
		*mod = LMOD_Z;
		return 1;
	case 't':
		*mod = LMOD_T;
		return 1;
	default:
		*mod = LMOD_NONE;
		return 0;
	}
}
/*****************************************************************************/
static int parse_conv(
	const char *restrict fmt,
	size_t pos,
	enum conv *conv
) {
	switch(fmt[pos]) {

	case 'd':
	case 'i':
		*conv = CONV_INT;
		return 1;
	case 'o':
		*conv = CONV_UINT_O;
		return 1;
	case 'u':
		*conv = CONV_UINT;
		return 1;
	case 'x':
		*conv = CONV_UINT_X;
		return 1;
	case 'X':
		*conv = CONV_UINT_XX;
		return 1;
	case 'e':
		*conv = CONV_FLOAT_E;
		return 1;
	case 'E':
		*conv = CONV_FLOAT_EE;
		return 1;
	case 'f':
		/* fallthrough */
	case 'F':
		*conv = CONV_FLOAT;
		return 1;
	case 'g':
		*conv = CONV_FLOAT_G;
		return 1;
	case 'G':
		*conv = CONV_FLOAT_GG;
		return 1;
	case 'a':
		*conv = CONV_FLOAT_X;
		return 1;
	case 'A':
		*conv = CONV_FLOAT_XX;
		return 1;
	case 'c':
		*conv = CONV_CHAR;
		return 1;
	case 's':
		*conv = CONV_STR;
		return 1;
	case 'p':
		*conv = CONV_POINT;
		return 1;
	case 'n':
		*conv = CONV_CCOUNT;
		return 1;
	case 'm':
		*conv = CONV_ERRNO;
		return 1;
	case '%':
		*conv = CONV_PERCENT;
		return 1;
	default:
		return 0;
	}
}
/*****************************************************************************/
static struct fmt_arg_list *init_arg_list(void)
{
	struct fmt_arg_list* ret = ghost_malloc(
		sheap,
		(
			sizeof(struct fmt_arg_list) +
	 		(FMT_ARG_LIST_INIT_LEN * sizeof(struct fmt_arg))
		)
	);

	if(ret != NULL) {
		ret->len = 0;
		ret->size = FMT_ARG_LIST_INIT_LEN;
	}

	return ret;
}
/*****************************************************************************/
static struct fmt_arg *insert_arg_list(struct fmt_arg_list **lptr)
{
	if((*lptr)->size == (*lptr)->len) {
		size_t new_size = (*lptr)->size * 2;
		size_t byte_size = (
			sizeof(struct fmt_arg_list) +
			(sizeof(struct fmt_arg) * new_size)
		);
		struct fmt_arg_list *temp = ghost_realloc(
			sheap, *lptr, byte_size
		);
		if(temp == NULL) {
			return NULL;
		}
		*lptr = temp;
		(*lptr)->size = new_size;
	}

	struct fmt_arg *ret = &(*lptr)->args[(*lptr)->len];

	ret->aidx = (*lptr)->len;

	ret->pos = 0;
	ret->prec = 0;
	ret->width = 0;
	ret->flags = 0;

	(*lptr)->len += 1;

	return ret;
}
/*****************************************************************************/
static struct fmt_arg *insert_mod_arg(struct fmt_arg_list **lptr, int pos)
{
	struct fmt_arg *arg = insert_arg_list(lptr);

	arg->pos = pos;
	arg->conv = CONV_MOD;

	return arg;
}
/*****************************************************************************/
static void sort_arg_list_by_aidx(struct fmt_arg_list *list)
{
	size_t len = list->len;
	bool sorted = true;

	for(int i = 1; i < len; i++) {
		if(list->args[i].aidx < list->args[i - 1].aidx) {
			sorted = false;
			break;
		}
	}

	if(sorted) {
		return;
	}

	for(int i = 0; i < len; i++) {
		for(int j = 1; j < len - i; j++) {
			if(list->args[j].aidx < list->args[j - 1].aidx) {
				struct fmt_arg tmp = list->args[j];
				list->args[j] = list->args[j - 1];
				list->args[j - 1] = tmp;
			}
		}
	}
}
/*****************************************************************************/
static void sort_arg_list_by_pos(struct fmt_arg_list *list)
{
	size_t len = list->len;
	bool sorted = true;

	for(int i = 1; i < len; i++) {
		if(list->args[i].aidx < list->args[i - 1].pos) {
			sorted = false;
			break;
		}
	}

	if(sorted) {
		return;
	}

	for(int i = 0; i < len; i++) {
		for(int j = 1; j < len - i; j++) {
			if(list->args[j].pos < list->args[j - 1].pos) {
				struct fmt_arg tmp = list->args[j];
				list->args[j] = list->args[j - 1];
				list->args[j - 1] = tmp;
			}
		}
	}
}
/*****************************************************************************/
static void destroy_arg_list(struct fmt_arg_list *list)
{
	ghost_free(sheap, list);
}
/*****************************************************************************/
static void load_args(struct fmt_arg_list *list, va_list args)
{
	sort_arg_list_by_pos(list);

	for(int i = 0; i < list->len; i++) {
		struct fmt_arg *a = &list->args[i];
		switch(a->conv) {
		case CONV_INT:
		case CONV_CCOUNT:
		case CONV_ERRNO:
		case CONV_MOD:
			switch(a->mod) {
     			case LMOD_NONE:
     			case LMOD_H:
     			case LMOD_HH:
     				a->val.i = va_arg(args, int);
				break;
     			case LMOD_Z:
				a->val.sst = va_arg(args, ssize_t);
				break;
     			case LMOD_T:
     				a->val.pd = va_arg(args, ptrdiff_t);
     				break;
     			case LMOD_L:
     				a->val.li = va_arg(args, long int);
				break;
			case LMOD_LL:
     				a->val.lli = va_arg(args, long long int);
				break;
     			case LMOD_J:
				a->val.im = va_arg(args, intmax_t);
				break;
			}
     			break;
		case CONV_UINT:
		case CONV_UINT_O:
		case CONV_UINT_X:
		case CONV_UINT_XX:
			switch(a->mod) {
			case LMOD_NONE:
			case LMOD_H:
			case LMOD_HH:
				a->val.u = va_arg(args, unsigned int);
				break;
			case LMOD_Z:
				a->val.st = va_arg(args, size_t);
				break;
			case LMOD_T:
				a->val.pd = va_arg(args, ptrdiff_t);
				break;
			case LMOD_L:
				a->val.lu = va_arg(args, unsigned long int);
				break;
			case LMOD_LL:
				a->val.llu = va_arg(args, unsigned long long);
				break;
			case LMOD_J:
				a->val.um = va_arg(args, uintmax_t);
				break;
			}
			break;
		case CONV_CHAR:
			a->val.i = va_arg(args, int);
			break;
		case CONV_STR:
		case CONV_POINT:
			a->val.p = va_arg(args, void*);
			break;
		case CONV_FLOAT:
		case CONV_FLOAT_E:
		case CONV_FLOAT_EE:
		case CONV_FLOAT_G:
		case CONV_FLOAT_GG:
		case CONV_FLOAT_X:
		case CONV_FLOAT_XX:
			switch(a->mod) {
			case LMOD_NONE:
			case LMOD_H:
			case LMOD_HH:
			case LMOD_L:
			case LMOD_J:
			case LMOD_Z:
			case LMOD_T:
				a->val.d = va_arg(args, double);
				break;
			case LMOD_LL:
				a->val.ld = va_arg(args, long double);
     			}
			break;
		case CONV_PERCENT:
			/* ignore, should not happen */
			assert(false);
			break;
		}
	}

	sort_arg_list_by_aidx(list);
}
/*****************************************************************************/
static void process_mod_args(struct fmt_arg_list *list)
{
	for(int i = 0; i < list->len; i++) {
		struct fmt_arg *a = &list->args[i];

		if(a->width < 0) {
			int pos = -1 * a->width;
			a->width = list->args[pos].val.i;
		}
		if(a->prec < 0 && a->prec != PREC_UNDEF) {
			int pos = -1 * a->prec;
			a->prec = list->args[pos].val.i;
		}
	}
}
/*****************************************************************************/
static const char *emit_str(
	const char *s,
	void(*emit)(void*, char),
	void *emit_arg
) {
	int i;

	for(i = 0; s[i] != '\0'; i++) {
		emit(emit_arg, s[i]);
	}

	return s + i + 1;
}
/*****************************************************************************/
static char digi_char(unsigned digi, bool upper)
{
	if(digi < 10) {
		return '0' + digi;
	} else if(upper) {
		return 'A' + (digi - 10);
	} else {
		return 'a' + (digi - 10);
	}
}
/*****************************************************************************/
static int uint_base(const struct fmt_arg *arg)
{
	if(arg->conv == CONV_UINT_O) {
		return 8;
	} else if(arg->conv == CONV_UINT_X || arg->conv == CONV_UINT_XX) {
		return 16;
	} else if(arg->conv == CONV_POINT) {
		return 16;
	} else {
		return 10;
	}
}
/*****************************************************************************/
static void emit_pad(char p, int m, void(*emit)(void*, char), void *emit_arg)
{
	for(int i = 0; i < m; i++) {
		emit(emit_arg, p);
	}
}
/*****************************************************************************/
static int emit_float(
	const struct fmt_arg *arg,
	void(*emit)(void*, char),
	void *emit_arg
) {
	long double v;

	if(arg->mod == LMOD_LL || arg->mod == LMOD_L) {
		v = arg->val.ld;
	} else {
		v = arg->val.d;
	}

	char conv = 'f';

	switch(arg->conv) {
	case CONV_FLOAT:
		conv = 'f';
		break;
	case CONV_FLOAT_E:
		conv = 'e';
		break;
	case CONV_FLOAT_EE:
		conv = 'E';
		break;
	case CONV_FLOAT_G:
		conv = 'g';
		break;
	case CONV_FLOAT_GG:
		conv = 'G';
		break;
	case CONV_FLOAT_X:
		conv = 'a';
		break;
	case CONV_FLOAT_XX:
		conv = 'A';
		break;
	default:
		assert(false);
		break;
	}

	int fl = 0;

	if(arg->flags & FLAG_ALT) {
		fl |= ALT_FORM;
	}
	if(arg->flags & FLAG_LADJ) {
		fl |= LEFT_ADJ;
	}
	if(arg->flags & FLAG_SSPC) {
		fl |= PAD_POS;
	}
	if(arg->flags & FLAG_ZPAD) {
		fl |= ZERO_PAD;
	}
	if(arg->flags & FLAG_PSIN) {
		fl |= MARK_POS;
	}

	struct musl_output_obj o;
	o.emit = emit;
	o.emit_arg = emit_arg;

	musl_fmt_fp(
		&o,
		v,
		arg->width,
		arg->prec,
		fl,
		conv
	);

	return 0;
}
/*****************************************************************************/
static int emit_arg_str(
	const struct fmt_arg *arg,
	void(*emit)(void*, char),
	void *emit_arg
) {
	if(arg->mod == LMOD_L) {
		const wchar_t *str = arg->val.p;
		char temp[8];
		mbstate_t ps;
		if(arg->prec == PREC_UNDEF) {
			for(int i = 0; str[i] != L'\0'; i++) {
				int c = wcrtomb(temp, str[i], &ps);
				assert(c <= 8);

				for(int j = 0; j < c; j++) {
					emit(emit_arg, temp[j]);
				}
			}
		} else {
			for(int i = 0; i < arg->prec; i++) {
				if(str[i] == L'\0') {
					break;
				}
				int c = wcrtomb(temp, str[i], &ps);
				assert(c <= 8);

				for(int j = 0; j < c; j++) {
					emit(emit_arg, temp[j]);
				}
			}
		}
	} else {
		const char *str = arg->val.p;

		if(arg->prec == PREC_UNDEF) {
			if(str != NULL) {
				emit_str(str, emit, emit_arg);
			} else {
				emit_str("NULL", emit, emit_arg);
			}
		} else {
			for(int i = 0; i < arg->prec; i++) {
				if(str[i] == '\0') {
					break;
				}
				emit(emit_arg, str[i]);
			}
		}
	}

	return 0;
}
/*****************************************************************************/
static int emit_char(
	const struct fmt_arg *arg,
	void(*emit)(void*, char),
	void *emit_arg
) {
	bool wide = arg->mod == LMOD_L;

	if(wide) {
		wchar_t w = (wchar_t)arg->val.i;
		if(w == L'\0') {
			emit(emit_arg, '\0');
		} else {
			char temp[8];
			mbstate_t ps;
			int c = wcrtomb(temp, w, &ps);
			assert(c < sizeof(temp));

			for(int i = 0; i < c; i++) {
				emit(emit_arg, temp[i]);
			}
		}
	} else {
		emit(emit_arg, arg->val.i);
	}

	return 0;
}
/*****************************************************************************/
static int emit_uint(
	const struct fmt_arg *arg,
	void(*emit)(void*, char),
	void *emit_arg
) {
	int base = uint_base(arg);
	uintmax_t val;

	if(arg->conv == CONV_ERRNO) {
		val = arg->val.u;
	} else if(arg->conv == CONV_POINT) {
		val = (size_t)arg->val.p;
	} else {
		switch(arg->mod) {
		case LMOD_NONE:
		case LMOD_H:
		case LMOD_HH:
			val = arg->val.u;
			break;
		case LMOD_L:
			val = arg->val.lu;
			break;
		case LMOD_LL:
			val = arg->val.llu;
			break;
		case LMOD_Z:
			val = arg->val.st;
		case LMOD_T:
			val = arg->val.pd;
			break;
		case LMOD_J:
			val = arg->val.um;
			break;
		default:
			val = 0;
			break;
		}
	}

	const char *prefix = "";
	if((arg->flags & FLAG_ALT || arg->conv == CONV_POINT) && val != 0) {
		if(base == 16 && (arg->conv == CONV_UINT_XX)) {
			prefix = "0X";
		} else if(base == 16) {
			prefix = "0x";
		} else if(base == 8 && (arg->conv == CONV_UINT_O)) {
			prefix = "0";
		} else if(arg->flags & FLAG_PSIN) {
			prefix = "+";
		} else if(arg->flags & FLAG_SSPC) {
			prefix = " ";
		}
	}

	char temp[print_width_uint_max_t(base) + 1];
	int idx = print_width_uint_max_t(base);

	bool upper = (arg->conv == CONV_UINT_XX);

	temp[idx] = '\0';

	if(val == 0) {
	} else {
		while(val != 0) {
			idx -= 1;
			temp[idx] = digi_char(val % base, upper);
			val /= base;
		}
	}

	char pad = arg->flags & FLAG_ZPAD ? '0' : ' ';
	bool ladj = arg->flags & FLAG_LADJ;

	int n_width = sizeof(temp) - idx - 1 + strlen(prefix);

	if(arg->width != 0 && arg->width > n_width) {
		if(pad == ' ' && !ladj) {
			emit_pad(pad, arg->width - n_width, emit, emit_arg);
		}
		emit_str(prefix, emit, emit_arg);
		if(pad == '0') {
			emit_pad(pad, arg->width - n_width, emit, emit_arg);
		}
		emit_str(temp + idx, emit, emit_arg);

		if(pad == ' ' && ladj) {
			emit_pad(pad, arg->width - n_width, emit, emit_arg);
		}
	} else {
		emit_str(prefix, emit, emit_arg);
		emit_str(temp + idx, emit, emit_arg);
	}

	return 0;
}
/*****************************************************************************/
static int emit_int(
	const struct fmt_arg *arg,
	void(*emit)(void*, char),
	void *emit_arg
) {
	const char *sign = "";
	char pad = ' ';
	intmax_t val;

	switch(arg->mod) {
	case LMOD_NONE:
	case LMOD_H:
	case LMOD_HH:
		val = arg->val.i;
		break;
	case LMOD_L:
		val = arg->val.li;
		break;
	case LMOD_LL:
		val = arg->val.lli;
		break;
	case LMOD_Z:
		val = arg->val.sst;
	case LMOD_T:
		val = arg->val.pd;
		break;
	case LMOD_J:
		val = arg->val.im;
		break;
	default:
		val = 0;
		break;
	}

	if(val < 0) {
		sign = "-";
	}
	else if(arg->flags & FLAG_PSIN) {
			sign = "+";
	} else if(arg->flags & FLAG_SSPC) {
			sign = " ";
	}

	if(arg->flags & FLAG_ZPAD) {
		pad = '0';
	}

	char temp[print_width_intmax_t() + 1];
	int idx = print_width_intmax_t();

	temp[idx] = '\0';

	int m = val < 0 ? -1 : 1;

	if(val == 0) {
		idx -= 1;
		temp[idx] = '0';
	} else {
		while(val != 0) {
			idx -= 1;
			temp[idx] = digi_char((val % 10) * m, false);
			val /= 10;
		}
	}

	int n_width = sizeof(temp) - idx - 1 + strlen(sign);

	bool ladj = arg->flags & FLAG_LADJ;

	if(arg->width != 0 && arg->width > n_width) {
		if(pad == ' ' && !ladj) {
			emit_pad(pad, arg->width - n_width, emit, emit_arg);
		}
		emit_str(sign, emit, emit_arg);
		if(pad == '0') {
			emit_pad(pad, arg->width - n_width, emit, emit_arg);
		}
		emit_str(temp + idx, emit, emit_arg);

		if(pad == ' ' && ladj) {
			emit_pad(pad, arg->width - n_width, emit, emit_arg);
		}
	} else {
		emit_str(sign, emit, emit_arg);
		emit_str(temp + idx, emit, emit_arg);
	}

	return 0;
}
/*****************************************************************************/
static void emit_argument(
	const struct fmt_arg *arg,
	void(*emit)(void*, char),
	void *emit_arg
) {
	switch(arg->conv) {
	case CONV_INT:
		emit_int(arg, emit, emit_arg);
		break;
	case CONV_UINT:
	case CONV_UINT_O:
	case CONV_UINT_X:
	case CONV_UINT_XX:
	case CONV_POINT:
	case CONV_ERRNO:
		emit_uint(arg, emit, emit_arg);
		break;
	case CONV_CHAR:
		emit_char(arg, emit, emit_arg);
		break;
	case CONV_STR:
		emit_arg_str(arg, emit, emit_arg);
		break;
	case CONV_FLOAT:
	case CONV_FLOAT_E:
	case CONV_FLOAT_EE:
	case CONV_FLOAT_G:
	case CONV_FLOAT_GG:
	case CONV_FLOAT_X:
	case CONV_FLOAT_XX:
		emit_float(arg, emit, emit_arg);
		break;
	case CONV_PERCENT:
		assert(false);
		break;
	case CONV_MOD:
	case CONV_CCOUNT:
		break;
	}
}
/*****************************************************************************/
static int fmt_write(
	const char *restrict fmt,
	void(*emit)(void*,char),
	void *emit_arg,
	va_list args
) {
	struct fmt_arg_list *list = init_arg_list();
	int ret = 0;


	size_t fmt_len = strlen(fmt);
	char *fmt_fixed_parts = ghost_malloc(sheap, fmt_len + 1);
	int fparts_idx = 0;

	size_t pos = 0;
	int arg_pos = 0;

	while(pos < fmt_len) {
		int temp;

		if(fmt[pos] != '%') {
			fmt_fixed_parts[fparts_idx] = fmt[pos];
			pos += 1;
			fparts_idx += 1;
			continue;
		}
		if(fmt[pos + 1] == '%') {
			pos += 2;
			fmt_fixed_parts[fparts_idx] = '%';
			fparts_idx += 1;
			continue;
		}
		fmt_fixed_parts[fparts_idx] = '\0';
		fparts_idx += 1;
		pos += 1;

		struct fmt_arg *a = insert_arg_list(&list);

		pos += parse_pos(fmt, pos, &arg_pos);
		a->pos = arg_pos;

		pos += parse_flags(fmt, pos, &a->flags);

		if((temp = parse_awidth(fmt, pos, &arg_pos)) != 0) {
			insert_mod_arg(&list, arg_pos);
			a->width = -1 * arg_pos;
			pos += temp;
		} else {
			pos += parse_fwidth(fmt, pos, &a->width);
		}

		if((temp = parse_aprec(fmt, pos, &arg_pos)) != 0) {
			insert_mod_arg(&list, arg_pos);
			a->prec = -1 * arg_pos;
			pos += temp;
		} else {
			pos += parse_fprec(fmt, pos, &a->prec);
		}

		pos += parse_length_mod(fmt, pos, &a->mod);
		pos += parse_conv(fmt, pos, &a->conv);
	}

	fmt_fixed_parts[fparts_idx] = '\0';
	fparts_idx += 1;


	load_args(list, args);
	process_mod_args(list);

	const char *fixed_ptr = emit_str(fmt_fixed_parts, emit, emit_arg);

	for(int i = 0; i < list->len; i++) {
		emit_argument(&list->args[i], emit, emit_arg);
		fixed_ptr = emit_str(fixed_ptr, emit, emit_arg);
	}

	destroy_arg_list(list);
	ghost_free(sheap, fmt_fixed_parts);

	return ret;
}
/*****************************************************************************/
static void emit_to_null(void *arg, char c)
{
	int *count = arg;

	*count += 1;
}
/*****************************************************************************/
static void emit_to_fixed_string(void *arg, char c)
{
	struct output_str *ostr = (struct output_str*)arg;

	if(ostr->i < ostr->len) {
		ostr->str[ostr->i] = c;
		ostr->i += 1;
	}
}
/*****************************************************************************/
static void emit_to_dyn_str(void *arg, char c)
{
	struct output_str *ostr = (struct output_str*)arg;

	if(ostr->i >= ostr->len) {
		ostr->len *= 2;
		ostr->str = ghost_realloc(sheap, ostr->str, ostr->len);
	}

	if(ostr->str == NULL) {
		return;
	}

	ostr->str[ostr->i] = c;
	ostr->i += 1;
}
/*****************************************************************************/
static void emit_to_file(void *arg, char c)
{
	struct output_file *of = arg;
	struct ghost_file *f = of->f;

	if(circ_buffer_capacity(&f->wb) == 0) {
		ghost_fflush(f);
	}

	circ_buffer_write(&f->wb, &c, 1);
	of->i += 1;

	if(f->flags & GIO_FLAG_LF && c == '\n') {
		ghost_fflush(f);
	}
}
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
int ghost_fprintf(struct ghost_file *f, const char *restrict fmt, ...)
{
	struct output_file of;

	of.i = 0;
	of.f = f;

	if(!(f->flags & GIO_FLAG_WRITE)) {
		return -1;
	}

	va_list args;
	va_start(args, fmt);

	fmt_write(fmt, emit_to_file, &of, args);

	va_end(args);

	if(!(f->flags & GIO_FLAG_BUF)) {
		ghost_fflush(f);
	}

	return of.i;
}
/*****************************************************************************/
int ghost_printf(const char *restrict fmt, ...)
{
	struct output_file of;

	of.i = 0;
	of.f = ghost_stdout;

	va_list args;
	va_start(args, fmt);

	fmt_write(fmt, emit_to_file, &of, args);

	va_end(args);

	if(!(ghost_stdout->flags & GIO_FLAG_BUF)) {
		ghost_fflush(ghost_stdout);
	}

	return of.i;
}
/*****************************************************************************/
int ghost_snprintf(
	char *restrict str,
	size_t size,
	const char *restrict fmt,
	...
) {
	va_list args;
	va_start(args, fmt);

	int ret = 0;

	if(size == 0) {
		int count = 0;
		fmt_write(fmt, emit_to_null, &count, args);

		ret = count + 1;
	} else {
		struct output_str ostr;
		ostr.str = str;
		ostr.i = 0;
		ostr.len = size;

		fmt_write(fmt, emit_to_fixed_string, &ostr, args);

		if(ostr.i < ostr.len) {
			ostr.str[ostr.i] = '\0';
			ostr.i += 1;
		}
		ret = ostr.i;
	}

	va_end(args);

	return ret;
}
/*****************************************************************************/
int ghost_sdprintf(
	char **str,
	size_t size,
	const char *restrict fmt,
	...
) {
	struct output_str ostr;
	va_list args;

	va_start(args, fmt);

	int ret = 0;

	if(size == 0) {
		assert(str == NULL);

		size = DYN_STR_INIT_LEN;
		*str = ghost_malloc(sheap, size);
	}

	if(*str == NULL) {
		return -1;
	}

	ostr.str = *str;
	ostr.i = 0;
	ostr.len = size;


	fmt_write(fmt, emit_to_dyn_str, &ostr, args);

	if(ostr.str == NULL) {
		ret = -1;
	} else {
		ostr.str[ostr.i] = '\0';
		ostr.i += 1;

		ret = ostr.i;
	}

	va_end(args);

	return ret;
}
/*****************************************************************************/
