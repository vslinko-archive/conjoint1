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

#include "tokenizer.h"
#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#define cj_check_whitespace(character) \
    (character == 0x20)

#define cj_check_line_terminator(character) \
    (character == 0x0A)

#define cj_check_numeric(character) \
    (character >= 0x30 && character <= 0x39)

#define cj_check_alpha(character) \
    ((character >= 0x41 && character <= 0x5A) || (character >= 0x61 && character <= 0x7A))

#define cj_check_comment_start(character) \
    (character == 0x23)

#define cj_check_identifier_start(character) \
    (cj_check_alpha(character))

#define cj_check_identifier_part(character) \
    (cj_check_alpha(character) || cj_check_numeric(character))

#define cj_check_character_quote(character) \
    (character == 0x27)

#define cj_check_string_quote(character) \
    (character == 0x22)

#define cj_token_value_init(token) \
    cj_string_init(token->value, 8)

#define cj_token_value_append(token, character) \
    cj_string_append(token->value, character, token->value_length, 8)

static char* token_type_strings[] = {
    "COMMENT",
    "KEYWORD",
    "IDENTIFIER",
    "PUNCTUATOR",
    "NULL_LITERAL",
    "BOOLEAN_LITERAL",
    "NUMERIC_LITERAL",
    "CHARACTER_LITERAL",
    "STRING_LITERAL",
    "END_OF_FILE"
};

static struct cj_source_position* cj_fixate_current_position(const struct cj_tokenization_process* process) {
	struct cj_source_position* position = malloc(sizeof(struct cj_source_position));
    assert(position);
	position->position = process->current_position;
	position->line = process->current_line_number;
	position->column = process->current_position - process->current_line_start_position;
	return position;
}

static void cj_skip_whitespaces(struct cj_tokenization_process* process) {
    wchar_t character;

    while (process->current_position < process->source_file->content_length) {
        character = process->source_file->content[process->current_position];

        if (cj_check_whitespace(character)) {
            process->current_position++;
        } else if (cj_check_line_terminator(character)) {
            process->current_position++;
            process->current_line_number++;
            process->current_line_start_position = process->current_position;
        } else {
            break;
        }
    }
}

static void cj_scan_comment(struct cj_token* token, struct cj_tokenization_process* process) {
    wchar_t character = process->source_file->content[process->current_position];
    assert(cj_check_comment_start(character));
    process->current_position++;

    cj_token_value_init(token);

    while (process->current_position < process->source_file->content_length) {
        character = process->source_file->content[process->current_position];

        if (cj_check_line_terminator(character)) {
            break;
        } else {
            process->current_position++;
            cj_token_value_append(token, character);
        }
    }

    token->type = COMMENT;
}

static void cj_scan_identifier(struct cj_token* token, struct cj_tokenization_process* process) {
    wchar_t character = process->source_file->content[process->current_position];
    assert(cj_check_identifier_start(character));
    process->current_position++;

    cj_token_value_init(token);
    cj_token_value_append(token, character);

    while (process->current_position < process->source_file->content_length) {
        character = process->source_file->content[process->current_position];

        if (cj_check_identifier_part(character)) {
            process->current_position++;
            cj_token_value_append(token, character);
        } else {
            break;
        }
    }

    if (wcscmp(token->value, L"let") == 0 || wcscmp(token->value, L"import") == 0 || wcscmp(token->value, L"from") == 0) {
        token->type = KEYWORD;
    } else if (wcscmp(token->value, L"null") == 0) {
        token->type = NULL_LITERAL;
    } else if (wcscmp(token->value, L"true") == 0 || wcscmp(token->value, L"false") == 0) {
        token->type = BOOLEAN_LITERAL;
    } else {
        token->type = IDENTIFIER;
    }
}

static void cj_scan_numeric_literal(struct cj_token* token, struct cj_tokenization_process* process) {
    wchar_t character = process->source_file->content[process->current_position];
    assert(cj_check_numeric(character));
    process->current_position++;

    cj_token_value_init(token);
    cj_token_value_append(token, character);

    while (process->current_position < process->source_file->content_length) {
        character = process->source_file->content[process->current_position];

        if (cj_check_numeric(character)) {
            process->current_position++;
            cj_token_value_append(token, character);
        } else {
            break;
        }
    }

    token->type = NUMERIC_LITERAL;
}

static void cj_scan_character_literal(struct cj_token* token, struct cj_tokenization_process* process) {
    wchar_t character = process->source_file->content[process->current_position];
    assert(cj_check_character_quote(character));
    process->current_position++;

    character = process->source_file->content[process->current_position];
    assert(!cj_check_character_quote(character));
    cj_token_value_init(token);
    cj_token_value_append(token, character);
    process->current_position++;

    character = process->source_file->content[process->current_position];
    assert(cj_check_character_quote(character));
    process->current_position++;

    token->type = CHARACTER_LITERAL;
}

static void cj_scan_string_literal(struct cj_token* token, struct cj_tokenization_process* process) {
    wchar_t character = process->source_file->content[process->current_position];
    assert(cj_check_string_quote(character));
    process->current_position++;

    cj_token_value_init(token);

    while (process->current_position < process->source_file->content_length) {
        character = process->source_file->content[process->current_position];

        if (cj_check_string_quote(character)) {
            process->current_position++;
            token->type = STRING_LITERAL;
            return;
        } else if (cj_check_line_terminator(character)) {
            process->current_position++;
            process->current_line_number++;
            process->current_line_start_position = process->current_position;
        } else {
            process->current_position++;
        }

        cj_token_value_append(token, character);
    }

    assert(NULL);
}

static void cj_scan_punctuator(struct cj_token* token, struct cj_tokenization_process* process) {
    wchar_t character = process->source_file->content[process->current_position];

    switch (character) {
        case 0x25: // %
        case 0x28: // (
        case 0x29: // )
        case 0x2A: // *
        case 0x2B: // +
        case 0x2C: // ,
        case 0x2D: // -
        case 0x2E: // .
        case 0x2F: // /
        case 0x3A: // :
        case 0x3B: // ;
        case 0x3F: // ?
        case 0x5B: // [
        case 0x5D: // ]
        case 0x5E: // ^
        case 0x7B: // {
        case 0x7D: // }
        case 0x7E: // ~
            cj_token_value_init(token);
            cj_token_value_append(token, character);
            process->current_position++;
            token->type = PUNCTUATOR;
            return;
    }

    if ((process->source_file->content_length - process->current_position) >= 3) {
        wchar_t* character3 = malloc(sizeof(wchar_t) * 4);
        assert(character3);
        wmemcpy(character3, process->source_file->content + process->current_position, 3);
        character3[3] = '\0';
        if (wcscmp(character3, L">>>") == 0) {
            process->current_position += 3;
            token->value = character3;
            token->type = PUNCTUATOR;
            return;
        }
        free(character3);
    }

    if ((process->source_file->content_length - process->current_position) >= 2) {
        wchar_t* character2 = malloc(sizeof(wchar_t) * 3);
        assert(character2);
        wmemcpy(character2, process->source_file->content + process->current_position, 2);
        character2[2] = '\0';
        if (wcscmp(character2, L"!=") == 0 || (wcschr(L"<>&|=", character2[0]) != NULL && character2[0] == character2[1])) {
            process->current_position += 2;
            token->value = character2;
            token->type = PUNCTUATOR;
            return;
        }
        free(character2);
    }

    if (wcschr(L"<>=!&|", character) != NULL) {
        cj_token_value_init(token);
        cj_token_value_append(token, character);
        process->current_position++;
        token->type = PUNCTUATOR;
        return;
    }

    assert(NULL);
}

struct cj_token* cj_read_next_token(struct cj_tokenization_process* process) {
	struct cj_token* token = malloc(sizeof(struct cj_token));
    assert(token);
    token->value_length = 0;
	token->start = cj_fixate_current_position(process);

    cj_skip_whitespaces(process);

    if (process->current_position >= process->source_file->content_length) {
        token->type = END_OF_FILE;
        cj_token_value_init(token);
        token->end = cj_fixate_current_position(process);
        return token;
    }

    wchar_t character = process->source_file->content[process->current_position];

    if (cj_check_comment_start(character)) {
        cj_scan_comment(token, process);
    } else if (cj_check_identifier_start(character)) {
        cj_scan_identifier(token, process);
    } else if (cj_check_numeric(character)) {
        cj_scan_numeric_literal(token, process);
    } else if (cj_check_character_quote(character)) {
        cj_scan_character_literal(token, process);
    } else if (cj_check_string_quote(character)) {
        cj_scan_string_literal(token, process);
    } else {
        cj_scan_punctuator(token, process);
    }

    token->end = cj_fixate_current_position(process);

	return token;
}

void cj_release_token(struct cj_token* token) {
    free(token->start);
    free(token->end);
    free(token->value);
    free(token);
}

void cj_print_token(const struct cj_token* token) {
	printf("TYPE: %s\n", token_type_strings[token->type]);
	wprintf(L"VALUE: `%ls`\n", token->value);
	printf("START: p %d l %d c %d\n", token->start->position, token->start->line, token->start->column);
	printf("END: p %d l %d c %d\n", token->end->position, token->end->line, token->end->column);
}
