#include <stdio.h>
#include <stdlib.h>

#include "tokenvec.h"
#include "lexer.h"
#include "symtable.h"

/* ------------------------------------------------------------------ */
/*  Operacoes internas do vetor dinamico                               */
/* ------------------------------------------------------------------ */

/* Coloca o vetor no estado inicial vazio. */
static void tv_init(TokenVec *v)
{
    v->data = NULL;
    v->size = 0;
    v->cap  = 0;
}

/*
 * Garante capacidade para pelo menos n tokens.
 * Quando precisa crescer, dobra a capacidade atual.
 */
static void tv_reserve(TokenVec *v, int n)
{
    if (n <= v->cap) return;

    int cap = v->cap ? v->cap : 16;
    while (cap < n)
        cap *= 2;

    Token *p = realloc(v->data, (size_t)cap * sizeof(Token));
    if (!p) {
        perror("realloc");
        exit(1);
    }

    v->data = p;
    v->cap  = cap;
}

/* Insere um token no final do vetor. */
static void tv_push(TokenVec *v, Token t)
{
    if (v->size + 1 > v->cap)
        tv_reserve(v, v->size + 1);
    v->data[v->size++] = t;
}

/* ------------------------------------------------------------------ */
/*  Tokenizacao completa do arquivo-fonte                              */
/* ------------------------------------------------------------------ */
TokenVec tokenize_to_vector(FILE *src)
{
    TokenVec v;
    tv_init(&v);

    /* O lexico precisa da tabela de simbolos com as palavras reservadas. */
    SymTable *st = symtable_create();
    symtable_load_keywords(st);

    LexerState *ls = lexer_create(src, st);

    for (;;) {
        Token t = getNextToken(ls);

        if (t.type == TK_ERROR) {
            fprintf(stderr, "Erro lexico na linha %d: %s\n",
                    t.line, t.lexeme);
            lexer_free(ls);
            symtable_free(st);
            tv_free(&v);
            exit(1);
        }

        tv_push(&v, t);

        if (t.type == TK_EOF)
            break;
    }

    lexer_free(ls);
    symtable_free(st);
    return v;
}

void tv_free(TokenVec *v)
{
    free(v->data);
    v->data = NULL;
    v->size = 0;
    v->cap  = 0;
}
