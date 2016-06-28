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

#include <string.h>

void* memset(void* _dst, int c, size_t n) {
    uint8_t* dst = _dst;
    while (n-- > 0) {
        *dst++ = c;
    }
    return _dst;
}

void* memcpy(void* _dst, const void* _src, size_t n) {
    uint8_t* dst = _dst;
    const uint8_t* src = _src;
    while (n-- > 0) {
        *dst++ = *src++;
    }
    return _dst;
}

int memcmp(const void* _a, const void* _b, size_t n) {
    const uint8_t* a = _a;
    const uint8_t* b = _b;
    while (n-- > 0) {
        int x = *a++ - *b++;
        if (x != 0) {
            return x;
        }
    }
    return 0;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while (*s++)
        len++;
    return len;
}
