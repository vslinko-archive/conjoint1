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

#include "source_file.h"
#include "tokenizer.h"

#include <stdio.h>

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Usage: %s SOURCE_FILE\n", argv[0]);
		return 1;
	}

	struct cj_source_file source_file = {
		.path = argv[1]
	};

	if (cj_read_source_file(&source_file) < 0) {
		printf("Unable to read file \"%s\"\n", source_file.path);
		return 2;
	}

    struct cj_tokenization_process tokenization_process = {
        .source_file = &source_file,
        .current_position = 0,
        .current_line_number = 0,
        .current_line_start_position = 0
    };
    struct cj_token* token;

    while (1) {
        token = cj_read_next_token(&tokenization_process);
        
        cj_print_token(token);
        printf("---------\n");

        if (token->type == END_OF_FILE) {
            cj_release_token(token);
            break;
        }
        
        cj_release_token(token);
    }
    
	return 0;
}
