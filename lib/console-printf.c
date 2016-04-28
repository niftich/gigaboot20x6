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

#include <printf.h>

#include <efi.h>
#include <efilib.h>
#include <goodies.h>

#define PCBUFMAX 126
// buffer is two larger to leave room for a \0 and room
// for a \r that may be added after a \n

typedef struct {
	int off;
	CHAR16 buf[PCBUFMAX + 2];
} _pcstate;

static int _printf_console_out(const char *str, size_t len, void *_state) {
	_pcstate *state = _state;
	CHAR16 *buf = state->buf;
	int i = state->off;
	int n = len;
	while (n > 0) {
		if (*str == '\n') {
			buf[i++] = '\r';
		}
		buf[i++] = *str++;
		if (i >= PCBUFMAX) {
			buf[i] = 0;
			gConOut->OutputString(gConOut, buf);
			i = 0;
		}
		n--;
	}
	state->off = i;
	return len;
}

int _printf(const char *fmt, ...) {
	va_list ap;
	_pcstate state;
	int r;
	state.off = 0;
	va_start(ap, fmt);
	r = _printf_engine(_printf_console_out, &state, fmt, ap);
	va_end(ap);
	if (state.off) {
		state.buf[state.off] = 0;
		gConOut->OutputString(gConOut, state.buf);
	}
	return r;
}

