#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

#define MAX_LEXEME 256

/* ------------------------------------------------------------------ */
/*  Tipos de token                                                      */
/* ------------------------------------------------------------------ */
typedef enum {
    /* Palavras reservadas */
    KW_PROGRAM, KW_VAR, KW_INTEGER, KW_REAL,
    KW_BEGIN, KW_END, KW_IF, KW_THEN, KW_ELSE,
    KW_WHILE, KW_DO,

    /* Literais */
    TK_ID,
    TK_NUM_INT,
    TK_NUM_REAL,

    /* Operadores */
    OP_EQ,   /* =  */
    OP_NE,   /* <> */
    OP_LT,   /* <  */
    OP_LE,   /* <= */
    OP_GT,   /* >  */
    OP_GE,   /* >= */
    OP_ADD,  /* +  */
    OP_MIN,  /* -  */
    OP_MUL,  /* *  */
    OP_DIV,  /* /  */
    OP_ASS,  /* := */

    /* Símbolos */
    SMB_SEM, /* ;  */
    SMB_COM, /* ,  */
    SMB_OPA, /* (  */
    SMB_CPA, /* )  */
    SMB_COL, /* :  */
    SMB_DOT, /* .  */
    SMB_OBC, /* {  (reservado, nunca emitido — abre comentário) */
    SMB_CBC, /* }  (reservado, nunca emitido — fecha comentário) */

    /* Especiais */
    TK_EOF,
    TK_ERROR
} TokenType;

/* ------------------------------------------------------------------ */
/*  Token                                                               */
/* ------------------------------------------------------------------ */
typedef struct {
    TokenType type;
    char      lexeme[MAX_LEXEME];
    int       line;
    int       col;
} Token;

/* ------------------------------------------------------------------ */
/*  Lexer (declaração opaca — implementação em lexer.c)                */
/* ------------------------------------------------------------------ */
struct SymTable;   /* forward declaration para evitar dependência circular */

typedef struct LexerState LexerState;

LexerState *lexer_create(FILE *src, struct SymTable *symtab);
void        lexer_free(LexerState *ls);
Token       getNextToken(LexerState *ls);
const char *token_type_name(TokenType t);

#endif
