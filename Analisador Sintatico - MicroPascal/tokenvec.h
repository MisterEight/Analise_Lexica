#ifndef TOKENVEC_H
#define TOKENVEC_H

#include <stdio.h>
#include "lexer.h"

/*
 * Vetor dinamico de tokens.
 *
 * Esta e a estrutura que liga o analisador lexico ao sintatico:
 * o lexico le o programa-fonte inteiro e guarda todos os tokens aqui,
 * e o sintatico apenas caminha sobre este vetor usando um indice.
 */
typedef struct {
    Token *data;   /* arranjo de tokens                       */
    int    size;   /* quantos tokens ja foram armazenados      */
    int    cap;    /* capacidade alocada (cresce dobrando)     */
} TokenVec;

/*
 * Le todo o conteudo do arquivo-fonte e devolve um TokenVec
 * com todos os tokens, terminando sempre com um token TK_EOF.
 * Se o lexico encontrar um token invalido, a mensagem e exibida
 * e o programa e encerrado.
 */
TokenVec tokenize_to_vector(FILE *src);

/* Libera a memoria ocupada pelo vetor. */
void tv_free(TokenVec *v);

#endif
