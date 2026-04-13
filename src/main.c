#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "symtable.h"

/* ------------------------------------------------------------------ */
/*  Derivar nome de arquivo de saída                                    */
/*  Ex.: "teste.pas" + ".lex" → "teste.lex"                           */
/*       "prog"      + ".lex" → "prog.lex"                            */
/* ------------------------------------------------------------------ */
static void make_outname(const char *input, const char *ext,
                         char *out, size_t outsz)
{
    /* copia o nome base sem a extensão original */
    const char *dot = strrchr(input, '.');
    size_t base_len = dot ? (size_t)(dot - input) : strlen(input);
    if (base_len >= outsz - 5) base_len = outsz - 5;
    memcpy(out, input, base_len);
    out[base_len] = '\0';
    strncat(out, ext, outsz - base_len - 1);
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo.pas>\n", argv[0]);
        return 1;
    }

    const char *input_path = argv[1];

    /* -- Abrir arquivo fonte -- */
    FILE *src = fopen(input_path, "r");
    if (!src) {
        perror(input_path);
        return 1;
    }

    /* -- Derivar e abrir arquivos de saída -- */
    char lex_path[512], ts_path[512], err_path[512];
    make_outname(input_path, ".lex", lex_path, sizeof(lex_path));
    make_outname(input_path, ".ts",  ts_path,  sizeof(ts_path));
    make_outname(input_path, ".err", err_path, sizeof(err_path));

    FILE *lex_f = fopen(lex_path, "w");
    FILE *ts_f  = fopen(ts_path,  "w");
    FILE *err_f = fopen(err_path, "w");

    if (!lex_f || !ts_f || !err_f) {
        perror("fopen saida");
        return 1;
    }

    /* -- Inicializar tabela de símbolos e lexer -- */
    SymTable   *st = symtable_create();
    symtable_load_keywords(st);

    LexerState *ls = lexer_create(src, st);

    /* -- Loop principal de tokens -- */
    int error_count = 0;

    while (1) {
        Token tok = getNextToken(ls);

        if (tok.type == TK_EOF)
            break;

        if (tok.type == TK_ERROR) {
            fprintf(err_f, "Linha %d, Coluna %d: %s\n",
                    tok.line, tok.col, tok.lexeme);
            error_count++;
        } else {
            fprintf(lex_f, "<%s, %s> %d %d\n",
                    token_type_name(tok.type),
                    tok.lexeme,
                    tok.line,
                    tok.col);
        }
    }

    /* -- Tabela de símbolos -- */
    symtable_print(st, ts_f);

    /* -- Resumo no stdout -- */
    printf("Tokens escritos em : %s\n", lex_path);
    printf("Tabela de simbolos : %s\n", ts_path);
    printf("Erros lexico       : %s (%d erro(s))\n", err_path, error_count);

    /* -- Limpeza -- */
    lexer_free(ls);
    symtable_free(st);
    fclose(src);
    fclose(lex_f);
    fclose(ts_f);
    fclose(err_f);

    return (error_count > 0) ? 1 : 0;
}
