#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"
#include "symtable.h"

/* ------------------------------------------------------------------ */
/*  Estado interno do lexer                                             */
/* ------------------------------------------------------------------ */
struct LexerState {
    FILE     *src;
    int       line;
    int       col;
    SymTable *symtab;
};

/* ------------------------------------------------------------------ */
/*  Helpers: advance / unadvance                                        */
/* ------------------------------------------------------------------ */

/*
 * advance — lê um caractere do arquivo e atualiza linha/coluna.
 * Retorna o caractere lido ou EOF.
 */
static int advance(LexerState *ls)
{
    int c = fgetc(ls->src);
    if (c == '\n') {
        ls->line++;
        ls->col = 0;
    } else {
        ls->col++;
    }
    return c;
}

/*
 * unadvance — devolve um caractere ao stream (máx. 1 por vez, garantia da C).
 * Desfaz a atualização de linha/coluna correspondente.
 */
static void unadvance(LexerState *ls, int c)
{
    if (c == EOF) return;
    ungetc(c, ls->src);
    if (c == '\n') {
        ls->line--;
        /* coluna da linha anterior não é restaurada — aceitável */
    } else {
        ls->col--;
    }
}

/* ------------------------------------------------------------------ */
/*  Sub-DFA numérico                                                    */
/*  Espelha o padrão de codigo_exemplo.txt                             */
/* ------------------------------------------------------------------ */
typedef enum {
    N_INT,    /* lendo dígitos inteiros         (final: NUM_INT)  */
    N_DOT,    /* viu '.', aguarda dígito        (não final)       */
    N_REAL,   /* lendo parte decimal            (final: NUM_REAL) */
    N_E,      /* viu 'E'                        (não final)       */
    N_ESIGN,  /* viu sinal após 'E'             (não final)       */
    N_EXP,    /* lendo expoente                 (final: NUM_REAL) */
    N_DEAD
} NumState;

static NumState step_num(NumState s, int c)
{
    switch (s) {
        case N_INT:
            if (isdigit(c)) return N_INT;
            if (c == '.')   return N_DOT;
            if (c == 'e')   return N_E;
            break;
        case N_DOT:
            if (isdigit(c)) return N_REAL;
            break;
        case N_REAL:
            if (isdigit(c)) return N_REAL;
            if (c == 'e')   return N_E;
            break;
        case N_E:
            if (c == '+' || c == '-') return N_ESIGN;
            if (isdigit(c))           return N_EXP;
            break;
        case N_ESIGN:
            if (isdigit(c)) return N_EXP;
            break;
        case N_EXP:
            if (isdigit(c)) return N_EXP;
            break;
        default:
            break;
    }
    return N_DEAD;
}

/* Retorna 1 se o estado numérico é final */
static int num_is_final(NumState s) {
    return s == N_INT || s == N_REAL || s == N_EXP;
}

/* ------------------------------------------------------------------ */
/*  Mapeamento TokenType → string (para arquivo .lex)                  */
/* ------------------------------------------------------------------ */
const char *token_type_name(TokenType t)
{
    switch (t) {
        case KW_PROGRAM:  return "KW_PROGRAM";
        case KW_VAR:      return "KW_VAR";
        case KW_INTEGER:  return "KW_INTEGER";
        case KW_REAL:     return "KW_REAL";
        case KW_BEGIN:    return "KW_BEGIN";
        case KW_END:      return "KW_END";
        case KW_IF:       return "KW_IF";
        case KW_THEN:     return "KW_THEN";
        case KW_ELSE:     return "KW_ELSE";
        case KW_WHILE:    return "KW_WHILE";
        case KW_DO:       return "KW_DO";
        case TK_ID:       return "TK_ID";
        case TK_NUM_INT:  return "TK_NUM_INT";
        case TK_NUM_REAL: return "TK_NUM_REAL";
        case OP_EQ:       return "OP_EQ";
        case OP_NE:       return "OP_NE";
        case OP_LT:       return "OP_LT";
        case OP_LE:       return "OP_LE";
        case OP_GT:       return "OP_GT";
        case OP_GE:       return "OP_GE";
        case OP_ADD:      return "OP_ADD";
        case OP_MIN:      return "OP_MIN";
        case OP_MUL:      return "OP_MUL";
        case OP_DIV:      return "OP_DIV";
        case OP_ASS:      return "OP_ASS";
        case SMB_SEM:     return "SMB_SEM";
        case SMB_COM:     return "SMB_COM";
        case SMB_OPA:     return "SMB_OPA";
        case SMB_CPA:     return "SMB_CPA";
        case SMB_COL:     return "SMB_COL";
        case SMB_DOT:     return "SMB_DOT";
        case SMB_OBC:     return "SMB_OBC";
        case SMB_CBC:     return "SMB_CBC";
        case TK_EOF:      return "TK_EOF";
        case TK_ERROR:    return "TK_ERROR";
        default:          return "UNKNOWN";
    }
}

/* ------------------------------------------------------------------ */
/*  Criação / destruição do lexer                                       */
/* ------------------------------------------------------------------ */
LexerState *lexer_create(FILE *src, struct SymTable *symtab)
{
    LexerState *ls = malloc(sizeof(LexerState));
    if (!ls) { perror("lexer_create"); exit(1); }
    ls->src    = src;
    ls->line   = 1;
    ls->col    = 0;
    ls->symtab = symtab;
    return ls;
}

void lexer_free(LexerState *ls)
{
    free(ls);
}

/* ------------------------------------------------------------------ */
/*  Macro auxiliar para montar Token simples                           */
/* ------------------------------------------------------------------ */
#define MAKE_TOKEN(t, lex, ln, cl) \
    (Token){ .type = (t), .line = (ln), .col = (cl), \
             .lexeme = "" }; \
    strncpy(_tok.lexeme, (lex), MAX_LEXEME - 1)

/* ------------------------------------------------------------------ */
/*  getNextToken — função principal do lexer                           */
/* ------------------------------------------------------------------ */
Token getNextToken(LexerState *ls)
{
    Token tok;
    int c;

RESTART:
    /* ---- 1. Pular espaços em branco ---- */
    while (1) {
        c = advance(ls);
        if (c == EOF) goto emit_eof;
        if (!isspace(c)) break;
    }

    /* ---- 2. Comentário: { ... } ---- */
    if (c == '{') {
        int comment_line = ls->line;
        int comment_col  = ls->col;
        while (1) {
            c = advance(ls);
            if (c == '}') goto RESTART;   /* comentário fechado */
            if (c == EOF) {
                /* erro: comentário não fechado */
                tok.type = TK_ERROR;
                tok.line = comment_line;
                tok.col  = comment_col;
                snprintf(tok.lexeme, MAX_LEXEME,
                         "comentario nao fechado");
                return tok;
            }
        }
    }

    /* ---- 3. Registrar posição inicial do token ---- */
    int tok_line = ls->line;
    int tok_col  = ls->col;

    /* ---- 4. Normaliza para minúsculas ---- */
    int lc = tolower(c);

    /* ================================================================ */
    /*  DESPACHO                                                         */
    /* ================================================================ */

    /* -- Identificador / Palavra reservada -- */
    if (isalpha(lc)) {
        char buf[MAX_LEXEME];
        int  len = 0;
        buf[len++] = (char)lc;

        while (1) {
            int nx = advance(ls);
            int nx_lc = tolower(nx);
            if (isalpha(nx_lc) || isdigit(nx_lc)) {
                if (len < MAX_LEXEME - 1)
                    buf[len++] = (char)nx_lc;
            } else {
                unadvance(ls, nx);
                break;
            }
        }
        buf[len] = '\0';

        SymEntry *entry = symtable_lookup_or_insert(ls->symtab, buf, TK_ID);
        tok.type = entry->type;
        tok.line = tok_line;
        tok.col  = tok_col;
        strncpy(tok.lexeme, buf, MAX_LEXEME - 1);
        tok.lexeme[MAX_LEXEME - 1] = '\0';
        return tok;
    }

    /* -- Número inteiro ou real -- */
    if (isdigit(lc)) {
        char buf[MAX_LEXEME];
        int  len = 0;
        buf[len++] = (char)lc;

        NumState ns = N_INT;

        while (1) {
            int nx = advance(ls);

            /* Tratamento especial para '.':
             * Precisamos saber se o próximo char após '.' é um dígito.
             * Se não for, o '.' pertence ao próximo token (ex: SMB_DOT em "end.").
             */
            if (nx == '.' && ns == N_INT) {
                int after_dot = advance(ls);
                if (isdigit(after_dot)) {
                    /* faz parte do número real */
                    buf[len++] = '.';
                    ns = N_DOT;
                    /* processa o dígito após o ponto */
                    ns = step_num(ns, after_dot);
                    buf[len++] = (char)after_dot;
                    continue;
                } else {
                    /* '.' não pertence ao número */
                    unadvance(ls, after_dot);
                    unadvance(ls, nx);
                    break;
                }
            }

            int nx_lc = (nx == 'E' || nx == 'e') ? 'e' : nx;
            NumState next_ns = step_num(ns, nx_lc);

            if (next_ns == N_DEAD) {
                unadvance(ls, nx);
                break;
            }

            ns = next_ns;
            if (len < MAX_LEXEME - 1)
                buf[len++] = (char)((nx == 'E') ? 'E' : nx);
        }
        buf[len] = '\0';

        if (!num_is_final(ns)) {
            tok.type = TK_ERROR;
            tok.line = tok_line;
            tok.col  = tok_col;
            snprintf(tok.lexeme, MAX_LEXEME, "numero malformado: %s", buf);
            return tok;
        }

        tok.type = (ns == N_INT) ? TK_NUM_INT : TK_NUM_REAL;
        tok.line = tok_line;
        tok.col  = tok_col;
        strncpy(tok.lexeme, buf, MAX_LEXEME - 1);
        tok.lexeme[MAX_LEXEME - 1] = '\0';
        return tok;
    }

    /* -- Operadores e símbolos com lookahead -- */
    if (c == '<') {
        int nx = advance(ls);
        if (nx == '=') {
            tok.type = OP_LE;
            strncpy(tok.lexeme, "<=", MAX_LEXEME - 1);
        } else if (nx == '>') {
            tok.type = OP_NE;
            strncpy(tok.lexeme, "<>", MAX_LEXEME - 1);
        } else {
            unadvance(ls, nx);
            tok.type = OP_LT;
            strncpy(tok.lexeme, "<", MAX_LEXEME - 1);
        }
        tok.line = tok_line; tok.col = tok_col;
        return tok;
    }

    if (c == '>') {
        int nx = advance(ls);
        if (nx == '=') {
            tok.type = OP_GE;
            strncpy(tok.lexeme, ">=", MAX_LEXEME - 1);
        } else {
            unadvance(ls, nx);
            tok.type = OP_GT;
            strncpy(tok.lexeme, ">", MAX_LEXEME - 1);
        }
        tok.line = tok_line; tok.col = tok_col;
        return tok;
    }

    if (c == ':') {
        int nx = advance(ls);
        if (nx == '=') {
            tok.type = OP_ASS;
            strncpy(tok.lexeme, ":=", MAX_LEXEME - 1);
        } else {
            unadvance(ls, nx);
            tok.type = SMB_COL;
            strncpy(tok.lexeme, ":", MAX_LEXEME - 1);
        }
        tok.line = tok_line; tok.col = tok_col;
        return tok;
    }

    /* -- Tokens de um único caractere -- */
    {
        TokenType single_type;
        char single_lex[3] = { (char)c, '\0', '\0' };
        int valid = 1;

        switch (c) {
            case '=': single_type = OP_EQ;  break;
            case '+': single_type = OP_ADD; break;
            case '-': single_type = OP_MIN; break;
            case '*': single_type = OP_MUL; break;
            case '/': single_type = OP_DIV; break;
            case ';': single_type = SMB_SEM; break;
            case ',': single_type = SMB_COM; break;
            case '(': single_type = SMB_OPA; break;
            case ')': single_type = SMB_CPA; break;
            case '.': single_type = SMB_DOT; break;
            case '}':
                /* '}' fora de comentário → erro */
                tok.type = TK_ERROR;
                tok.line = tok_line;
                tok.col  = tok_col;
                snprintf(tok.lexeme, MAX_LEXEME,
                         "caractere invalido '}'");
                return tok;
            default:
                valid = 0;
                break;
        }

        if (valid) {
            tok.type = single_type;
            tok.line = tok_line;
            tok.col  = tok_col;
            strncpy(tok.lexeme, single_lex, MAX_LEXEME - 1);
            tok.lexeme[MAX_LEXEME - 1] = '\0';
            return tok;
        }
    }

    /* -- Caractere inválido -- */
    tok.type = TK_ERROR;
    tok.line = tok_line;
    tok.col  = tok_col;
    snprintf(tok.lexeme, MAX_LEXEME, "caractere invalido '%c'", c);
    return tok;

emit_eof:
    tok.type = TK_EOF;
    tok.line = ls->line;
    tok.col  = ls->col;
    tok.lexeme[0] = '\0';
    return tok;
}
