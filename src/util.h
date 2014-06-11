/* Copyright (c) 2014 Vyacheslav Slinko
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef CONJOINT_SRC_UTIL_H_
#define CONJOINT_SRC_UTIL_H_

#include <stdlib.h>
#include <wchar.h>

#define cj_string_init(string, chunk_size) \
    string = malloc(sizeof(wchar_t) * (chunk_size + 1)); \
    assert(string); \
    string[0] = '\0';

#define cj_string_append(string, character, length, chunk_size) \
    if (length > 0 && length % chunk_size == 0) { \
        string = realloc(string, sizeof(wchar_t) * (length + chunk_size + 1)); \
        assert(string); \
    } \
    string[length++] = character; \
    string[length] = '\0';

#endif /* CONJOINT_SRC_UTIL_H_ */
