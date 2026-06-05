#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtable.h"

/* ------------------------------------------------------------------ */
/*  Hash djb2-xor                                                       */
/* ------------------------------------------------------------------ */
static unsigned int sym_hash(const char *s)
{
    unsigned int h = 5381;
    while (*s)
        h = ((h << 5) + h) ^ (unsigned char)(*s++);
    return h % SYM_SIZE;
}

/* ------------------------------------------------------------------ */
/*  Criação / destruição                                                */
/* ------------------------------------------------------------------ */
SymTable *symtable_create(void)
{
    SymTable *st = calloc(1, sizeof(SymTable));
    if (!st) { perror("symtable_create"); exit(1); }
    return st;
}

void symtable_free(SymTable *st)
{
    for (int i = 0; i < SYM_SIZE; i++) {
        SymEntry *e = st->buckets[i];
        while (e) {
            SymEntry *next = e->next;
            free(e);
            e = next;
        }
    }
    free(st);
}

/* ------------------------------------------------------------------ */
/*  Lookup / insert                                                     */
/* ------------------------------------------------------------------ */
SymEntry *symtable_lookup_or_insert(SymTable *st, const char *lexeme,
                                    TokenType type)
{
    unsigned int h = sym_hash(lexeme);
    SymEntry *e = st->buckets[h];

    /* Busca */
    while (e) {
        if (strcmp(e->lexeme, lexeme) == 0)
            return e;   /* já existe — retorna sem alterar */
        e = e->next;
    }

    /* Inserção */
    SymEntry *novo = malloc(sizeof(SymEntry));
    if (!novo) { perror("symtable_lookup_or_insert"); exit(1); }
    strncpy(novo->lexeme, lexeme, MAX_LEXEME - 1);
    novo->lexeme[MAX_LEXEME - 1] = '\0';
    novo->type = type;
    novo->next = st->buckets[h];
    st->buckets[h] = novo;
    return novo;
}

/* ------------------------------------------------------------------ */
/*  Pré-carga das palavras reservadas                                   */
/* ------------------------------------------------------------------ */
static const struct { const char *lex; TokenType type; } KEYWORDS[] = {
    { "program", KW_PROGRAM },
    { "var",     KW_VAR     },
    { "integer", KW_INTEGER },
    { "real",    KW_REAL    },
    { "begin",   KW_BEGIN   },
    { "end",     KW_END     },
    { "if",      KW_IF      },
    { "then",    KW_THEN    },
    { "else",    KW_ELSE    },
    { "while",   KW_WHILE   },
    { "do",      KW_DO      },
};
#define NUM_KW (int)(sizeof(KEYWORDS) / sizeof(KEYWORDS[0]))

void symtable_load_keywords(SymTable *st)
{
    for (int i = 0; i < NUM_KW; i++)
        symtable_lookup_or_insert(st, KEYWORDS[i].lex, KEYWORDS[i].type);
}

/* ------------------------------------------------------------------ */
/*  Impressão                                                           */
/* ------------------------------------------------------------------ */
void symtable_print(SymTable *st, FILE *out)
{
    fprintf(out, "%-20s %s\n", "Lexema", "Tipo");
    fprintf(out, "%-20s %s\n", "------", "----");
    for (int i = 0; i < SYM_SIZE; i++) {
        for (SymEntry *e = st->buckets[i]; e; e = e->next)
            fprintf(out, "%-20s %s\n", e->lexeme,
                    token_type_name(e->type));
    }
}
