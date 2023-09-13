/*
This code was originally written by musl libc.

musl as a whole is licensed under the following standard MIT license:

----------------------------------------------------------------------
Copyright © 2005-2020 Rich Felker, et al.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
----------------------------------------------------------------------*/
/******************************************************************************
*                                  INCLUDES                                   *
******************************************************************************/
#include <gio/musl-fmt-double.h>

#include <stdint.h>
#include <string.h>
#include <limits.h>
/******************************************************************************
*                                   DEFINES                                   *
******************************************************************************/
#define __BYTE_ORDER byte_order()
#define __LITTLE_ENDIAN 0xddccbbaaU
#define __BIG_ENDIAN 0xaabbccddU

#define LDBL_TRUE_MIN 3.6451995318824746025e-4951L
#define LDBL_MIN     3.3621031431120935063e-4932L
#define LDBL_MAX     1.1897314953572317650e+4932L
#define LDBL_EPSILON 1.0842021724855044340e-19L

#define LDBL_MANT_DIG 64
#define LDBL_MIN_EXP (-16381)
#define LDBL_MAX_EXP 16384

#define LDBL_DIG 18
#define LDBL_MIN_10_EXP (-4931)
#define LDBL_MAX_10_EXP 4932

#define FP_NAN       0
#define FP_INFINITE  1
#define FP_ZERO      2
#define FP_SUBNORMAL 3
#define FP_NORMAL    4

#define DECIMAL_DIG 21

#define signbit(x) ( \
	sizeof(x) == sizeof(float) ? (int)(__FLOAT_BITS(x)>>31) : \
	sizeof(x) == sizeof(double) ? (int)(__DOUBLE_BITS(x)>>63) : \
	__signbitl(x) )

#define isfinite(x) ( \
	sizeof(x) == sizeof(float) ? \
		(__FLOAT_BITS(x) & 0x7fffffff) < 0x7f800000 : \
	sizeof(x) == sizeof(double) ? \
		(__DOUBLE_BITS(x) & -1ULL>>1) < 0x7ffULL<<52 : \
	__fpclassifyl(x) > FP_INFINITE)

#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))

/******************************************************************************
*                                    TYPES                                    *
******************************************************************************/
union ldshape {
	long double f;
	struct {
		uint64_t m;
		uint16_t se;
	} i;
};
/******************************************************************************
*                                    DATA                                     *
******************************************************************************/
static const char xdigits[16] = {
	"0123456789ABCDEF"
};
/******************************************************************************
*                              STATIC FUNCTIONS                               *
******************************************************************************/
static unsigned byte_order(void)
{
	uint32_t u = 0xaabbccddu;
	uint8_t *p = (uint8_t*)&u;

	return
		p[0] << 24 |
		p[1] << 16 |
		p[2] << 8  |
		p[3] << 0  ;
}
/*****************************************************************************/
static int __fpclassifyl(long double x)
{
	union ldshape u = {x};
	int e = u.i.se & 0x7fff;
	int msb = u.i.m>>63;
	if (!e && !msb)
		return u.i.m ? FP_SUBNORMAL : FP_ZERO;
	if (e == 0x7fff) {
		/* The x86 variant of 80-bit extended precision only admits
		 * one representation of each infinity, with the mantissa msb
		 * necessarily set. The version with it clear is invalid/nan.
		 * The m68k variant, however, allows either, and tooling uses
		 * the version with it clear. */
		if (__BYTE_ORDER == __LITTLE_ENDIAN && !msb)
			return FP_NAN;
		return u.i.m << 1 ? FP_NAN : FP_INFINITE;
	}
	if (!msb)
		return FP_NAN;
	return FP_NORMAL;
}
/*****************************************************************************/
static int __signbitl(long double x)
{
	union ldshape u = {x};
	return u.i.se >> 15;
}
/*****************************************************************************/
long double frexpl(long double x, int *e)
{
	union ldshape u = {x};
	int ee = u.i.se & 0x7fff;

	if (!ee) {
		if (x) {
			x = frexpl(x*0x1p120, e);
			*e -= 120;
		} else *e = 0;
		return x;
	} else if (ee == 0x7fff) {
		return x;
	}

	*e = ee - 0x3ffe;
	u.i.se &= 0x8000;
	u.i.se |= 0x3ffe;
	return u.f;
}
/*****************************************************************************/
static inline unsigned __FLOAT_BITS(float __f)
{
	union {float __f; unsigned __i;} __u;
	__u.__f = __f;
	return __u.__i;
}
/*****************************************************************************/
static inline unsigned long long __DOUBLE_BITS(double __f)
{
	union {double __f; unsigned long long __i;} __u;
	__u.__f = __f;
	return __u.__i;
}
/*****************************************************************************/
static void out(struct musl_output_obj *o, const char *s, size_t l)
{
	for(int i = 0; i < l; i++) {
		if(s[i] == '\0') {
			return;
		}
		o->emit(o->emit_arg, s[i]);
	}
}
/*****************************************************************************/
static void pad(struct musl_output_obj *o, char c, int w, int l, int fl)
{
	char pad[256];
	if (fl & (LEFT_ADJ | ZERO_PAD) || l >= w) return;
	l = w - l;
	memset(pad, c, l>sizeof pad ? sizeof pad : l);
	for (; l >= sizeof pad; l -= sizeof pad)
		out(o, pad, sizeof pad);
	out(o, pad, l);
}
/*****************************************************************************/
static char *fmt_u(uintmax_t x, char *s)
{
	unsigned long y;
	for (   ; x>ULONG_MAX; x/=10) *--s = '0' + x%10;
	for (y=x;           y; y/=10) *--s = '0' + y%10;
	return s;
}
/******************************************************************************
*                            FUNCTION DEFINITIONS                             *
******************************************************************************/
int musl_fmt_fp(
	struct musl_output_obj *out_o,
	long double y,
	int w,
	int p,
	int fl,
	int t
) {
	uint32_t big[(LDBL_MANT_DIG+28)/29 + 1          // mantissa expansion
		+ (LDBL_MAX_EXP+LDBL_MANT_DIG+28+8)/9]; // exponent expansion
	uint32_t *a, *d, *r, *z;
	int e2=0, e, i, j, l;
	char buf[9+LDBL_MANT_DIG/4], *s;
	const char *prefix="-0X+0X 0X-0x+0x 0x";
	int pl;
	char ebuf0[3*sizeof(int)], *ebuf=&ebuf0[3*sizeof(int)], *estr;

	pl=1;
	if (signbit(y)) {
		y=-y;
	} else if (fl & MARK_POS) {
		prefix+=3;
	} else if (fl & PAD_POS) {
		prefix+=6;
	} else prefix++, pl=0;

	if (!isfinite(y)) {
		char *s = (t&32)?"inf":"INF";
		if (y!=y) s=(t&32)?"nan":"NAN";
		pad(out_o, ' ', w, 3+pl, fl&~ZERO_PAD);
		out(out_o, prefix, pl);
		out(out_o, s, 3);
		pad(out_o, ' ', w, 3+pl, fl^LEFT_ADJ);
		return MAX(w, 3+pl);
	}

	y = frexpl(y, &e2) * 2;
	if (y) e2--;

	if ((t|32)=='a') {
		long double round = 8.0;
		int re;

		if (t&32) prefix += 9;
		pl += 2;

		if (p<0 || p>=LDBL_MANT_DIG/4-1) re=0;
		else re=LDBL_MANT_DIG/4-1-p;

		if (re) {
			round *= 1<<(LDBL_MANT_DIG%4);
			while (re--) round*=16;
			if (*prefix=='-') {
				y=-y;
				y-=round;
				y+=round;
				y=-y;
			} else {
				y+=round;
				y-=round;
			}
		}

		estr=fmt_u(e2<0 ? -e2 : e2, ebuf);
		if (estr==ebuf) *--estr='0';
		*--estr = (e2<0 ? '-' : '+');
		*--estr = t+('p'-'a');

		s=buf;
		do {
			int x=y;
			*s++=xdigits[x]|(t&32);
			y=16*(y-x);
			if (s-buf==1 && (y||p>0||(fl&ALT_FORM))) *s++='.';
		} while (y);

		if (p > INT_MAX-2-(ebuf-estr)-pl)
			return -1;
		if (p && s-buf-2 < p)
			l = (p+2) + (ebuf-estr);
		else
			l = (s-buf) + (ebuf-estr);

		pad(out_o, ' ', w, pl+l, fl);
		out(out_o, prefix, pl);
		pad(out_o, '0', w, pl+l, fl^ZERO_PAD);
		out(out_o, buf, s-buf);
		pad(out_o, '0', l-(ebuf-estr)-(s-buf), 0, 0);
		out(out_o, estr, ebuf-estr);
		pad(out_o, ' ', w, pl+l, fl^LEFT_ADJ);
		return MAX(w, pl+l);
	}
	if (p<0) p=6;

	if (y) y *= 0x1p28, e2-=28;

	if (e2<0) a=r=z=big;
	else a=r=z=big+sizeof(big)/sizeof(*big) - LDBL_MANT_DIG - 1;

	do {
		*z = y;
		y = 1000000000*(y-*z++);
	} while (y);

	while (e2>0) {
		uint32_t carry=0;
		int sh=MIN(29,e2);
		for (d=z-1; d>=a; d--) {
			uint64_t x = ((uint64_t)*d<<sh)+carry;
			*d = x % 1000000000;
			carry = x / 1000000000;
		}
		if (carry) *--a = carry;
		while (z>a && !z[-1]) z--;
		e2-=sh;
	}
	while (e2<0) {
		uint32_t carry=0, *b;
		int sh=MIN(9,-e2), need=1+(p+LDBL_MANT_DIG/3U+8)/9;
		for (d=a; d<z; d++) {
			uint32_t rm = *d & ((1<<sh)-1);
			*d = (*d>>sh) + carry;
			carry = (1000000000>>sh) * rm;
		}
		if (!*a) a++;
		if (carry) *z++ = carry;
		/* Avoid (slow!) computation past requested precision */
		b = (t|32)=='f' ? r : a;
		if (z-b > need) z = b+need;
		e2+=sh;
	}

	if (a<z) for (i=10, e=9*(r-a); *a>=i; i*=10, e++);
	else e=0;

	/* Perform rounding: j is precision after the radix (possibly neg) */
	j = p - ((t|32)!='f')*e - ((t|32)=='g' && p);
	if (j < 9*(z-r-1)) {
		uint32_t x;
		/* We avoid C's broken division of negative numbers */
		d = r + 1 + ((j+9*LDBL_MAX_EXP)/9 - LDBL_MAX_EXP);
		j += 9*LDBL_MAX_EXP;
		j %= 9;
		for (i=10, j++; j<9; i*=10, j++);
		x = *d % i;
		/* Are there any significant digits past j? */
		if (x || d+1!=z) {
			long double round = 2/LDBL_EPSILON;
			long double small;
			if ((*d/i & 1) || (i==1000000000 && d>a && (d[-1]&1)))
				round += 2;
			if (x<i/2) small=0x0.8p0;
			else if (x==i/2 && d+1==z) small=0x1.0p0;
			else small=0x1.8p0;
			if (pl && *prefix=='-') round*=-1, small*=-1;
			*d -= x;
			/* Decide whether to round by probing round+small */
			if (round+small != round) {
				*d = *d + i;
				while (*d > 999999999) {
					*d--=0;
					if (d<a) *--a=0;
					(*d)++;
				}
				for (i=10, e=9*(r-a); *a>=i; i*=10, e++);
			}
		}
		if (z>d+1) z=d+1;
	}
	for (; z>a && !z[-1]; z--);

	if ((t|32)=='g') {
		if (!p) p++;
		if (p>e && e>=-4) {
			t--;
			p-=e+1;
		} else {
			t-=2;
			p--;
		}
		if (!(fl&ALT_FORM)) {
			/* Count trailing zeros in last place */
			if (z>a && z[-1]) {
				for (i=10, j=0; z[-1]%i==0; i*=10, j++);
			}
			else {
				j=9;
			}
			if ((t|32)=='f')
				p = MIN(p,MAX(0,9*(z-r-1)-j));
			else
				p = MIN(p,MAX(0,9*(z-r-1)+e-j));
		}
	}
	if (p > INT_MAX-1-(p || (fl&ALT_FORM)))
		return -1;
	l = 1 + p + (p || (fl&ALT_FORM));
	if ((t|32)=='f') {
		if (e > INT_MAX-l) return -1;
		if (e>0) l+=e;
	} else {
		estr=fmt_u(e<0 ? -e : e, ebuf);
		while(ebuf-estr<2) *--estr='0';
		*--estr = (e<0 ? '-' : '+');
		*--estr = t;
		if (ebuf-estr > INT_MAX-l) return -1;
		l += ebuf-estr;
	}

	if (l > INT_MAX-pl) return -1;
	pad(out_o, ' ', w, pl+l, fl);
	out(out_o, prefix, pl);
	pad(out_o, '0', w, pl+l, fl^ZERO_PAD);

	if ((t|32)=='f') {
		if (a>r) a=r;
		for (d=a; d<=r; d++) {
			char *s = fmt_u(*d, buf+9);
			if (d!=a) while (s>buf) *--s='0';
			else if (s==buf+9) *--s='0';
			out(out_o, s, buf+9-s);
		}
		if (p || (fl&ALT_FORM)) out(out_o, ".", 1);
		for (; d<z && p>0; d++, p-=9) {
			char *s = fmt_u(*d, buf+9);
			while (s>buf) *--s='0';
			out(out_o, s, MIN(9,p));
		}
		pad(out_o, '0', p+9, 9, 0);
	} else {
		if (z<=a) z=a+1;
		for (d=a; d<z && p>=0; d++) {
			char *s = fmt_u(*d, buf+9);
			if (s==buf+9) *--s='0';
			if (d!=a) while (s>buf) *--s='0';
			else {
				out(out_o, s++, 1);
				if (p>0||(fl&ALT_FORM)) out(out_o, ".", 1);
			}
			out(out_o, s, MIN(buf+9-s, p));
			p -= buf+9-s;
		}
		pad(out_o, '0', p+18, 18, 0);
		out(out_o, estr, ebuf-estr);
	}

	pad(out_o, ' ', w, pl+l, fl^LEFT_ADJ);

	return MAX(w, pl+l);
}
/*****************************************************************************/
