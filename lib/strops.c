/*** strops.c -- useful string operations
 *
 * Copyright (C) 2011-2012 Sebastian Freundt
 *
 * Author:  Sebastian Freundt <freundt@ga-group.nl>
 *
 * This file is part of dateutils.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the author nor the names of any contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **/
/* implementation part of strops.h */
#if !defined INCLUDED_strops_c_
#define INCLUDED_strops_c_

#include <stdbool.h>
#include <string.h>
/* for strncasecmp() */
#include <strings.h>
#include "nifty.h"
#include "strops.h"

#if defined __INTEL_COMPILER
/* we MUST return a char* */
# pragma warning (disable:2203)
#elif defined __GNUC__
# pragma GCC diagnostic ignored "-Wcast-qual"
#endif	/* __INTEL_COMPILER */

/* stolen from Klaus Klein/David Laight's strptime() */
DEFUN int32_t
strtoi_lim(const char *str, const char **ep, int32_t llim, int32_t ulim)
{
	int32_t res = 0;
	const char *sp;
	/* we keep track of the number of digits via rulim */
	int32_t rulim;

	for (sp = str, rulim = ulim > 10 ? ulim : 10;
	     res * 10 <= ulim && rulim && *sp >= '0' && *sp <= '9';
	     sp++, rulim /= 10) {
		res *= 10;
		res += *sp - '0';
	}
	if (UNLIKELY(sp == str)) {
		res = -1;
	} else if (UNLIKELY(res < llim || res > ulim)) {
		res = -2;
	}
	*ep = (char*)sp;
	return res;
}

DEFUN int32_t
strtoi(const char *str, const char **ep)
{
	const char *sp = str;
	bool negp = false;
	int32_t res;

	if (*str == '-') {
		negp = true;
		sp++;
	}
	if ((res = strtoi_lim(sp, ep, 0, INT32_MAX)) < 0) {
		*ep = str;
	} else if (negp) {
		res = -res;
	}
	return res;
}

DEFUN size_t
ui32tostr(char *restrict buf, size_t bsz, uint32_t d, int pad)
{
/* all strings should be little */
#define C(x, d)	(char)((x) / (d) % 10 + '0')
	size_t res;

	if (UNLIKELY(d > 1000000000)) {
		return 0;
	}
	switch ((res = (size_t)pad) < bsz ? res : bsz) {
	case 9:
		/* for nanoseconds */
		buf[pad - 9] = C(d, 100000000);
		buf[pad - 8] = C(d, 10000000);
		buf[pad - 7] = C(d, 1000000);
	case 6:
		/* for microseconds */
		buf[pad - 6] = C(d, 100000);
		buf[pad - 5] = C(d, 10000);
	case 4:
		/* for western year numbers */
		buf[pad - 4] = C(d, 1000);
	case 3:
		/* for milliseconds or doy et al. numbers */
		buf[pad - 3] = C(d, 100);
	case 2:
		/* hours, mins, secs, doms, moys, etc. */
		buf[pad - 2] = C(d, 10);
	case 1:
		buf[pad - 1] = C(d, 1);
		break;
	default:
	case 0:
		res = 0;
		break;
	}
	return res;
}


/* roman numerals */
static uint32_t
__romstr_v(const char c)
{
	switch (c) {
	case 'n':
	case 'N':
		return 0;
	case 'i':
	case 'I':
		return 1;
	case 'v':
	case 'V':
		return 5;
	case 'x':
	case 'X':
		return 10;
	case 'l':
	case 'L':
		return 50;
	case 'c':
	case 'C':
		return 100;
	case 'd':
	case 'D':
		return 500;
	case 'm':
	case 'M':
		return 1000;
	default:
		return -1U;
	}
}

DEFUN int32_t
romstrtoi_lim(const char *str, const char **ep, int32_t llim, int32_t ulim)
{
	int32_t res = 0;
	const char *sp;
	uint32_t v;

	/* loops through characters */
	for (sp = str, v = __romstr_v(*sp); *sp; sp++) {
		uint32_t nv = __romstr_v(sp[1]);

		if (UNLIKELY(v == -1U)) {
			break;
		} else if (LIKELY(nv == -1U || v >= nv)) {
			res += v;
		} else {
			res -= v;
		}
		v = nv;
	}
	if (UNLIKELY(sp == str)) {
		res = -1;
	} else if (UNLIKELY(res < llim || res > ulim)) {
		res = -2;
	}
	*ep = (char*)sp;
	return res;
}

static size_t
__rom_pr1(char *buf, size_t bsz, unsigned int i, char cnt, char hi, char lo)
{
	size_t res = 0;

	if (UNLIKELY(bsz < 4)) {
		return 0;
	}
	switch (i) {
	case 9:
		buf[res++] = cnt;
		buf[res++] = hi;
		break;
	case 4:
		buf[res++] = cnt;
		buf[res++] = lo;
		break;
	case 8:
		buf[++res] = cnt;
	case 7:
		buf[++res] = cnt;
	case 6:
		buf[++res] = cnt;
	case 5:
		buf[0] = lo;
		res++;
		break;
	case 3:
		buf[res++] = cnt;
	case 2:
		buf[res++] = cnt;
	case 1:
		buf[res++] = cnt;
		break;
	default:
		buf[res] = '\0';
		break;
	}
	return res;
}

DEFUN size_t
ui32tostrrom(char *restrict buf, size_t bsz, uint32_t d)
{
	size_t res;

	for (res = 0; d >= 1000 && res < bsz; d -= 1000) {
		buf[res++] = 'M';
	}

	res += __rom_pr1(buf + res, bsz - res, d / 100U, 'C', 'M', 'D');
	d %= 100;
	res += __rom_pr1(buf + res, bsz - res, d / 10U, 'X', 'C', 'L');
	d %= 10;
	res += __rom_pr1(buf + res, bsz - res, d, 'I', 'X', 'V');
	return res;
}


DEFUN int
__ordinalp(const char *num, size_t off_suf, char **ep)
{
#define __tolower(c)	(c | 0x20)
#define ILEA(a, b)	(((a) << 8) | (b))
	const char *p = num + off_suf;
	int res = 0;
	int p2;

	if (UNLIKELY(off_suf == 0 || p[0] == '\0')) {
		res = -1;
		goto yep;
	} else if ((p2 = ILEA(__tolower(p[0]), __tolower(p[1]))),
		   LIKELY(p2 == ILEA('t', 'h'))) {
		/* we accept 1th 2th 3th */
		p += 2;
		goto yep;
	} else if (UNLIKELY(off_suf >= 2 && p[-2] == '1')) {
		res = -1;
		goto yep;
	}
	/* irregular ordinals */
	switch (p[-1]) {
	case '1':
		if (p2 == ILEA('s', 't')) {
			p += 2;
		}
		break;
	case '2':
		if (p2 == ILEA('n', 'd')) {
			p += 2;
		}
		break;
	case '3':
		if (p2 == ILEA('r', 'd')) {
			p += 2;
		}
		break;
	default:
		res = -1;
		break;
	}
yep:
	*ep = (char*)p;
	return res;
#undef ILEA
#undef __tolower
}

DEFUN size_t
__ordtostr(char *buf, size_t bsz)
{
	char *p = buf;

	if (UNLIKELY(bsz < 2)) {
		return 0;
	}
	/* assumes the actual number is printed in BUF already, 2 digits long */
	if (UNLIKELY(p[-2] == '1')) {
		/* must be 11, 12, or 13 then */
		goto teens;
	} else if (p[-2] == '0') {
		/* discard */
		p[-2] = p[-1];
		p--;
	}
	switch (p[-1]) {
	default:
	teens:
		*p++ = 't';
		*p++ = 'h';
		break;
	case '1':
		*p++ = 's';
		*p++ = 't';
		break;
	case '2':
		*p++ = 'n';
		*p++ = 'd';
		break;
	case '3':
		*p++ = 'r';
		*p++ = 'd';
		break;
	}
	return p - buf;
}


/* string array funs */
DEFUN uint32_t
strtoarri(const char *buf, const char **ep, const char *const *arr, size_t narr)
{
/* take a string, compare it to an array of string (case-insensitively) and
 * return its index if found or -1U if not */
	for (size_t i = 0; i < narr; i++) {
		const char *chk = arr[i];
		size_t len = strlen(chk);

		if (strncasecmp(chk, buf, len) == 0) {
			if (ep != NULL) {
				*ep = buf + len;
			}
			return i;
		}
	}
	/* no matches */
	if (ep != NULL) {
		*ep = buf;
	}
	return -1U;
}

DEFUN size_t
arritostr(
	char *restrict buf, size_t bsz, size_t i,
	const char *const *arr, size_t narr)
{
/* take a string array, an index into the array and print the string
 * behind it into BUF, return the number of bytes copied */
	size_t ncp;
	size_t len;

	if (i > narr) {
		return 0;
	}
	len = strlen(arr[i]);
	ncp = bsz > len ? len : bsz;
	memcpy(buf, arr[i], ncp);
	return ncp;
}


/* faster strpbrk, strspn and strcspn, code by Richard A. O'Keefe
 * comp.unix.programmer Message-ID: <5449jv$p21$1@goanna.cs.rmit.edu.au>#1/1 */

#define ALPHABET_SIZE	(256)

/* not reentrant */
static unsigned char table[ALPHABET_SIZE];
static unsigned char cycle = 0;

static inline bool
in_current_set(unsigned char c)
{
	return table[c] == cycle;
}

static inline void
set_up_table(const unsigned char *set, bool include_NUL)
{
	if (LIKELY(set != NULL)) {
		/* useful for strtok() too */
                if (UNLIKELY(cycle == ALPHABET_SIZE - 1)) {
			memset(table, 0, sizeof(table));
			cycle = (unsigned char)1;
                } else {
			cycle = (unsigned char)(cycle + 1);
                }
                while (*set) {
			table[*set++] = cycle;
		}
	}
	table[0] = (unsigned char)(include_NUL ? cycle : 0);
	return;
}

DEFUN size_t
xstrspn(const char *src, const char *set)
{
	size_t i;

	set_up_table((const unsigned char*)set, false);
	for (i = 0; in_current_set((unsigned char)src[i]); i++);
	return i;
}

DEFUN size_t
xstrcspn(const char *src, const char *set)
{
	size_t i;

	set_up_table((const unsigned char*)set, true);
	for (i = 0; !in_current_set((unsigned char)src[i]); i++);
	return i;
}

DEFUN char*
xstrpbrk(const char *src, const char *set)
{
	const char *p;

	set_up_table((const unsigned char*)set, true);
	for (p = src; !in_current_set((unsigned char)*p); p++);
	return (char*)p;
}

DEFUN char*
xstrpbrkp(const char *src, const char *set, size_t *set_offs)
{
	const char *p;

	set_up_table((const unsigned char*)set, true);
	for (p = src; !in_current_set((unsigned char)*p); p++);
	if (LIKELY(set_offs != NULL)) {
		size_t idx;
		for (idx = 0; set[idx] != *p; idx++);
		*set_offs = idx;
	}
	return (char*)p;
}

#if defined __INTEL_COMPILER
# pragma warning (default:2203)
#elif defined __GNUC__
# pragma GCC diagnostic warning "-Wcast-qual"
#endif	/* __INTEL_COMPILER */

#endif	/* INCLUDED_strops_c_ */
