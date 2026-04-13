#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdio.h>
#include "lexer.h"

#define SYM_SIZE 64

typedef struct SymEntry {
    char             lexeme[MAX_LEXEME];
    TokenType        type;
    struct SymEntry *next;
} SymEntry;

typedef struct SymTable {
    SymEntry *buckets[SYM_SIZE];
} SymTable;

SymTable *symtable_create(void);
void      symtable_free(SymTable *st);

/* Pré-carrega as 11 palavras reservadas da linguagem */
void      symtable_load_keywords(SymTable *st);

/*
 * Busca 'lexeme' na tabela.
 *   - Se encontrado, retorna a entrada existente (sem alterar o tipo).
 *   - Se não encontrado, insere com o tipo 'type' e retorna a nova entrada.
 * lexeme deve estar em letras minúsculas.
 */
SymEntry *symtable_lookup_or_insert(SymTable *st, const char *lexeme,
                                    TokenType type);

/* Imprime a tabela no formato: Lexema\tTipo\n */
void      symtable_print(SymTable *st, FILE *out);

#endif
