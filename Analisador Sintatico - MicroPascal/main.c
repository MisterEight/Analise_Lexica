/*
 * Analisador Sintatico do MicroPascal
 * Linguagens Formais, Automatos e Compiladores (UCB, 1 semestre de 2026)
 * Grupo: Leonardo Silva de Alcantara e Marcelo Henrique
 */

#include <stdio.h>
#include <stdlib.h>

#include "tokenvec.h"
#include "sintatico.h"

/*
 * Corpo principal do compilador.
 *
 * Le o programa-fonte indicado na linha de comando, gera o vetor de tokens
 * (analise lexica) e em seguida chama o analisador sintatico, que reconhece
 * a entrada a partir do simbolo inicial <programa> e imprime a sequencia de
 * regras de producao usadas.
 */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo.pas>\n", argv[0]);
        return 1;
    }

    FILE *src = fopen(argv[1], "r");
    if (!src) {
        perror(argv[1]);
        return 1;
    }

    /* 1) Analise lexica: tokeniza o fonte inteiro para o vetor. */
    TokenVec tv = tokenize_to_vector(src);
    fclose(src);

    /* 2) Analise sintatica: percorre o vetor a partir de <programa>. */
    analisar(&tv);

    tv_free(&tv);
    return 0;
}
