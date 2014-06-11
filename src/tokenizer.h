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

#ifndef CONJOINT_SRC_TOKENIZER_H_
#define CONJOINT_SRC_TOKENIZER_H_

#include "source_file.h"

#include <wchar.h>

enum cj_token_type {
	COMMENT,
	KEYWORD,
	IDENTIFIER,
	PUNCTUATOR,
	NULL_LITERAL,
	BOOLEAN_LITERAL,
	NUMERIC_LITERAL,
	CHARACTER_LITERAL,
	STRING_LITERAL,
	END_OF_FILE
};

struct cj_source_position {
	int position;
	int line;
	int column;
};

struct cj_token {
	enum cj_token_type type;

    int value_length;
	wchar_t* value;

	struct cj_source_position* start;
	struct cj_source_position* end;
};

struct cj_tokenization_process {
	struct cj_source_file* source_file;
	int current_position;
	int current_line_number;
	int current_line_start_position;
};

struct cj_token* cj_read_next_token(struct cj_tokenization_process* process);

void cj_release_token(struct cj_token* token);

void cj_print_token(const struct cj_token* token);

#endif /* CONJOINT_SRC_TOKENIZER_H_ */
