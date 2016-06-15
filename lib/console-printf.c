// Copyright 2016 The Fuchsia Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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

