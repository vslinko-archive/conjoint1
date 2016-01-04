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

#include "parser.h"
#include "tokenizer.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct cj_ast_tree_node {
    char* type;

    int childrens_length;
    struct cj_ast_tree_node_children** childrens;
};

enum cj_ast_tree_node_children_type {
    NODE_TYPE,
    STRING_TYPE,
    NUMBER_TYPE,
    CHARACTER_TYPE,
    BOOLEAN_TYPE,
    NULL_TYPE
};

struct cj_ast_tree_node_children {
    enum cj_ast_tree_node_children_type type;

    char* name;

    union {
        struct cj_ast_tree_node* node;
        wchar_t* string;
        long double number;
        wchar_t character;
        bool boolean;
    };
};

struct cj_parsing_process {
    struct cj_tokenization_process* tokenization_process;
    struct cj_token* next_token;
};

static void cj_print_ast(struct cj_ast_tree_node* root, int level) {
    char* indent = malloc(sizeof(char) * level * 4 + 1);
    memset(indent, ' ', level * 4);
    indent[level * 4] = '\0';
    printf("%s", indent);
    printf("TYPE: %s\n", root->type);
    printf("%s", indent);
    if (root->childrens_length > 0) {
        printf("CHILDRENS:\n");
        for (int i = 0; i < root->childrens_length; i++) {
            printf("%s", indent);
            printf("    %s:", root->childrens[i]->name);
            switch (root->childrens[i]->type) {
                case NODE_TYPE:
                    printf("\n");
                    cj_print_ast(root->childrens[i]->node, level + 2);
                    break;

                case STRING_TYPE:
                    wprintf(L" \"%ls\"\n", root->childrens[i]->string);
                    break;

                case NUMBER_TYPE:
                    wprintf(L" %Lf\n", root->childrens[i]->number);
                    break;

                case CHARACTER_TYPE:
                    wprintf(L" '%lc'\n", root->childrens[i]->character);
                    break;

                case BOOLEAN_TYPE:
                    if (root->childrens[i]->boolean) {
                        printf(" true\n");
                    } else {
                        printf(" false\n");
                    }
                    break;

                case NULL_TYPE:
                    printf(" null\n");
                    break;
            }
        }
    } else {
        printf("CHILDRENS: ~\n");
    }
    free(indent);
}

static struct cj_ast_tree_node* cj_init_ast_tree_node(char* type) {
    struct cj_ast_tree_node* node = malloc(sizeof(struct cj_ast_tree_node));
    node->type = type;
    node->childrens_length = 0;
    return node;
}

static void cj_attach_ast_tree_node_children(struct cj_ast_tree_node* parent, struct cj_ast_tree_node_children *children) {
    if (parent->childrens_length == 0) {
        parent->childrens = malloc(sizeof(struct cj_ast_tree_node_children*));
    } else {
        parent->childrens = realloc(parent->childrens, sizeof(struct cj_ast_tree_node_children*) * parent->childrens_length + 1);
    }
    parent->childrens[parent->childrens_length++] = children;
}

static void cj_add_ast_tree_node_relation(struct cj_ast_tree_node* parent, struct cj_ast_tree_node* related, char* relation_name) {
    struct cj_ast_tree_node_children* relation = malloc(sizeof(struct cj_ast_tree_node_children));
    relation->type = NODE_TYPE;
    relation->name = relation_name;
    relation->node = related;
    cj_attach_ast_tree_node_children(parent, relation);
}

static void cj_add_ast_tree_node_string_value(struct cj_ast_tree_node* parent, char* relation_name, wchar_t* string) {
    struct cj_ast_tree_node_children* relation = malloc(sizeof(struct cj_ast_tree_node_children));
    relation->type = STRING_TYPE;
    relation->name = relation_name;
    relation->string = string;
    cj_attach_ast_tree_node_children(parent, relation);
}

static void cj_add_ast_tree_node_character_value(struct cj_ast_tree_node* parent, char* relation_name, wchar_t character) {
    struct cj_ast_tree_node_children* relation = malloc(sizeof(struct cj_ast_tree_node_children));
    relation->type = CHARACTER_TYPE;
    relation->name = relation_name;
    relation->character = character;
    cj_attach_ast_tree_node_children(parent, relation);
}

static void cj_add_ast_tree_node_boolean_value(struct cj_ast_tree_node* parent, char* relation_name, bool boolean) {
    struct cj_ast_tree_node_children* relation = malloc(sizeof(struct cj_ast_tree_node_children));
    relation->type = BOOLEAN_TYPE;
    relation->name = relation_name;
    relation->boolean = boolean;
    cj_attach_ast_tree_node_children(parent, relation);
}

static void cj_add_ast_tree_node_number_value(struct cj_ast_tree_node* parent, char* relation_name, long double number) {
    struct cj_ast_tree_node_children* relation = malloc(sizeof(struct cj_ast_tree_node_children));
    relation->type = NUMBER_TYPE;
    relation->name = relation_name;
    relation->number = number;
    cj_attach_ast_tree_node_children(parent, relation);
}

static void cj_add_ast_tree_node_null_value(struct cj_ast_tree_node* parent, char* relation_name) {
    struct cj_ast_tree_node_children* relation = malloc(sizeof(struct cj_ast_tree_node_children));
    relation->type = NULL_TYPE;
    relation->name = relation_name;
    cj_attach_ast_tree_node_children(parent, relation);
}

static void cj_get_next_token(struct cj_parsing_process* process) {
    if (process->next_token != NULL) {
        cj_release_token(process->next_token);
    }
    process->next_token = cj_read_next_token(process->tokenization_process);
}

static void cj_expect_keyword(struct cj_parsing_process* process, wchar_t* keyword) {
    assert(process->next_token->type == KEYWORD);
    assert(wcscmp(process->next_token->value, keyword) == 0);
    cj_get_next_token(process);
}

static void cj_expect_punctuator(struct cj_parsing_process* process, wchar_t* punctuator) {
    assert(process->next_token->type == PUNCTUATOR);
    assert(wcscmp(process->next_token->value, punctuator) == 0);
    cj_get_next_token(process);
}

static struct cj_ast_tree_node* cj_parse_comment(struct cj_parsing_process* process) {
    assert(process->next_token->type == COMMENT);
    struct cj_ast_tree_node* comment = cj_init_ast_tree_node("Comment");
    cj_add_ast_tree_node_string_value(comment, "content", process->next_token->value);
    cj_get_next_token(process);
    return comment;
}

static struct cj_ast_tree_node* cj_parse_identifier(struct cj_parsing_process* process) {
    assert(process->next_token->type == IDENTIFIER);
    struct cj_ast_tree_node* id = cj_init_ast_tree_node("Identifier");
    cj_add_ast_tree_node_string_value(id, "value", process->next_token->value);
    cj_get_next_token(process);
    return id;
}

static struct cj_ast_tree_node* cj_parse_literal(struct cj_parsing_process* process) {
    struct cj_ast_tree_node* literal = cj_init_ast_tree_node("Literal");
    long double value;

    switch (process->next_token->type) {
        case STRING_LITERAL:
            cj_add_ast_tree_node_string_value(literal, "value", process->next_token->value);
            cj_get_next_token(process);
            break;

        case NUMERIC_LITERAL:
            swscanf(process->next_token->value, L"%Lf", &value);
            cj_add_ast_tree_node_number_value(literal, "value", value);
            cj_get_next_token(process);
            break;

        case CHARACTER_LITERAL:
            cj_add_ast_tree_node_character_value(literal, "value", process->next_token->value[0]);
            cj_get_next_token(process);
            break;

        case BOOLEAN_LITERAL:
            cj_add_ast_tree_node_boolean_value(literal, "value", wcscmp(process->next_token->value, L"true") == 0);
            cj_get_next_token(process);
            break;

        case NULL_LITERAL:
            cj_add_ast_tree_node_null_value(literal, "value");
            cj_get_next_token(process);
            break;

        default:
            assert(NULL);
    }

    return literal;
}

static bool cj_match_punctuator(struct cj_parsing_process* process, wchar_t* punctuator) {
    return process->next_token->type == PUNCTUATOR && wcscmp(process->next_token->value, punctuator) == 0;
}

static struct cj_ast_tree_node* cj_parse_primary_expression(struct cj_parsing_process* process) {
    switch (process->next_token->type) {
        case IDENTIFIER:
            return cj_parse_identifier(process);

        case STRING_LITERAL:
        case NUMERIC_LITERAL:
        case CHARACTER_LITERAL:
        case BOOLEAN_LITERAL:
        case NULL_LITERAL:
            return cj_parse_literal(process);

        default:
            assert(NULL);
    }
}

static struct cj_ast_tree_node* cj_parse_import_declaration(struct cj_parsing_process* process) {
    cj_expect_keyword(process, L"import");
    cj_expect_punctuator(process, L"{");

    struct cj_ast_tree_node* import_declaration = cj_init_ast_tree_node("ImportDeclaration");

    while (1) {
        struct cj_ast_tree_node* specifier = cj_parse_identifier(process);
        cj_add_ast_tree_node_relation(import_declaration, specifier, "specifier");

        if (cj_match_punctuator(process, L",")) {
            cj_get_next_token(process);
        } else {
            break;
        }
    }

    cj_expect_punctuator(process, L"}");
    cj_expect_keyword(process, L"from");

    assert(process->next_token->type == STRING_LITERAL);
    struct cj_ast_tree_node* source = cj_parse_literal(process);
    cj_add_ast_tree_node_relation(import_declaration, source, "source");

    cj_expect_punctuator(process, L";");

    return import_declaration;
}

static struct cj_ast_tree_node* cj_parse_variable_declaration(struct cj_parsing_process* process) {
    cj_expect_keyword(process, L"let");

    struct cj_ast_tree_node* variable_declaration = cj_init_ast_tree_node("VariableDeclaration");

    struct cj_ast_tree_node* id = cj_parse_identifier(process);
    cj_add_ast_tree_node_relation(variable_declaration, id, "id");

    cj_expect_punctuator(process, L":");

    struct cj_ast_tree_node* type = cj_parse_identifier(process);
    cj_add_ast_tree_node_relation(variable_declaration, type, "type");

    if (cj_match_punctuator(process, L"?")) {
        cj_get_next_token(process);
        cj_add_ast_tree_node_boolean_value(variable_declaration, "optional", true);
    } else {
        cj_add_ast_tree_node_boolean_value(variable_declaration, "optional", false);
    }

    cj_expect_punctuator(process, L"=");

    struct cj_ast_tree_node* init = cj_parse_primary_expression(process);
    cj_add_ast_tree_node_relation(variable_declaration, init, "init");

    cj_expect_punctuator(process, L";");

    return variable_declaration;
}

static struct cj_ast_tree_node* cj_parse_program_element(struct cj_parsing_process* process) {
    if (process->next_token->type == COMMENT) {
        return cj_parse_comment(process);
    } else if (process->next_token->type == KEYWORD) {
        if (wcscmp(process->next_token->value, L"import") == 0) {
            return cj_parse_import_declaration(process);
        } else if (wcscmp(process->next_token->value, L"let") == 0) {
            return cj_parse_variable_declaration(process);
        }
    }

    return NULL;
}

static struct cj_ast_tree_node* cj_parse_program(struct cj_parsing_process* process) {
    struct cj_ast_tree_node* program = cj_init_ast_tree_node("Program");

    while (process->next_token->type != END_OF_FILE) {
        struct cj_ast_tree_node* program_element = cj_parse_program_element(process);
        cj_add_ast_tree_node_relation(program, program_element, "body");
    }

    return program;
}

void cj_free_ast_tree_node(struct cj_ast_tree_node* node) {
    for (int i = 0; i < node->childrens_length; i++) {
//        free(node->childrens[i]->name);
        switch (node->childrens[i]->type) {
            case NODE_TYPE:
                cj_free_ast_tree_node(node->childrens[i]->node);
                break;

            case STRING_TYPE:
                //free(node->childrens[i]->value.string);
                break;

            default:
                break;
        }
        free(node->childrens[i]);
    }
    free(node->childrens);
    free(node);
}

void cj_parse(struct cj_source_file* source_file) {
    struct cj_tokenization_process tokenization_process = {
        .source_file = source_file,
        .current_position = 0,
        .current_line_number = 0,
        .current_line_start_position = 0
    };

    struct cj_parsing_process parsing_process = {
        .tokenization_process = &tokenization_process
    };

    cj_get_next_token(&parsing_process);

    struct cj_ast_tree_node* program = cj_parse_program(&parsing_process);
//    cj_print_ast(program, 0);

    cj_free_ast_tree_node(program);
    cj_release_token(parsing_process.next_token);
}
