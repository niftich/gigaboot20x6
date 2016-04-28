// Copyright 2016 Google Inc. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files
// (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software,
// and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <string.h>

void *memset(void *_dst, int c, size_t n) {
	uint8_t *dst = _dst;
	while (n-- > 0) {
		*dst++ = c;
	}
	return _dst;
}

void *memcpy(void *_dst, const void *_src, size_t n) {
	uint8_t *dst = _dst;
	const uint8_t *src = _src;
	while (n-- > 0) {
		*dst++ = *src++;
	}
	return _dst;
}

int memcmp(const void *_a, const void *_b, size_t n) {
	const uint8_t *a = _a;
	const uint8_t *b = _b;
	while (n-- > 0) {
		int x = *a++ - *b++;
		if (x != 0) {
			return x;
		}
	}
	return 0;
}

size_t strlen(const char *s) {
	size_t len = 0;
	while (*s++) len++;
	return len;
}
