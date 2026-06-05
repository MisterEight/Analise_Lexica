#ifndef SINTATICO_H
#define SINTATICO_H

#include "tokenvec.h"

/*
 * Estado do analisador sintatico.
 *
 * toks aponta para o vetor de tokens produzido pelo lexico,
 * i e a posicao do token corrente e n e a quantidade de tokens.
 */
typedef struct {
    const Token *toks;
    int i;
    int n;
} Parser;

/*
 * Ponto de entrada do analisador sintatico.
 * Recebe o vetor de tokens e tenta reconhecer um <programa> do MicroPascal.
 * Imprime a sequencia de regras de producao aplicadas.
 * Em caso de erro, exibe a mensagem no formato exigido e encerra.
 */
void analisar(const TokenVec *v);

#endif
