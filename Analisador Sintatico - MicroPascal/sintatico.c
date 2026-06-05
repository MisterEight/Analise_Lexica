#include <stdio.h>
#include <stdlib.h>

#include "sintatico.h"
#include "lexer.h"

/* ------------------------------------------------------------------ */
/*  Apoio do parser                                                    */
/* ------------------------------------------------------------------ */

/* Devolve o tipo do token corrente. */
static TokenType tipoCorrente(Parser *p)
{
    return p->toks[p->i].type;
}

/* Imprime a regra de producao que esta sendo aplicada. */
static void regra(const char *r)
{
    printf("%s\n", r);
}

/*
 * Emite a mensagem de erro sintatico no formato exigido e encerra.
 *   nn:token nao esperado [lex].
 *   nn:fim de arquivo nao esperado.
 */
static void erroSintatico(Parser *p)
{
    const Token *t = &p->toks[p->i];

    if (t->type == TK_EOF)
        printf("%d:fim de arquivo nao esperado.\n", t->line);
    else
        printf("%d:token nao esperado [%s].\n", t->line, t->lexeme);

    exit(1);
}

/*
 * CasaToken: compara o token corrente com o token esperado pela gramatica.
 * Se forem iguais, consome o token avancando para o proximo.
 * Caso contrario, emite o erro sintatico e encerra a compilacao.
 */
static void CasaToken(Parser *p, TokenType esperado)
{
    if (tipoCorrente(p) == esperado)
        p->i++;
    else
        erroSintatico(p);
}

/* Indica se o token corrente pode iniciar um <comando>. */
static int ehInicioComando(Parser *p)
{
    switch (tipoCorrente(p)) {
        case TK_ID:
        case KW_BEGIN:
        case KW_IF:
        case KW_WHILE:
            return 1;
        default:
            return 0;
    }
}

/* Indica se o token corrente e um operador relacional. */
static int ehRelacao(Parser *p)
{
    switch (tipoCorrente(p)) {
        case OP_EQ:
        case OP_NE:
        case OP_LT:
        case OP_LE:
        case OP_GE:
        case OP_GT:
            return 1;
        default:
            return 0;
    }
}

/* ------------------------------------------------------------------ */
/*  Declaracoes antecipadas (a gramatica e recursiva)                  */
/* ------------------------------------------------------------------ */
static void bloco(Parser *p);
static void parteDeclaracoes(Parser *p);
static void declaracaoVariaveis(Parser *p);
static void listaIdentificadores(Parser *p);
static void tipo(Parser *p);
static void comandoComposto(Parser *p);
static void comando(Parser *p);
static void atribuicao(Parser *p);
static void comandoCondicional(Parser *p);
static void comandoRepetitivo(Parser *p);
static void expressao(Parser *p);
static void relacao(Parser *p);
static void expressaoSimples(Parser *p);
static void termo(Parser *p);
static void fator(Parser *p);
static void variavel(Parser *p);

/* ------------------------------------------------------------------ */
/*  Procedimentos: um para cada nao-terminal da gramatica              */
/* ------------------------------------------------------------------ */

/* <programa> ::= program <identificador> ; <bloco> . */
static void programa(Parser *p)
{
    regra("<programa> ::= program <identificador> ; <bloco> .");
    CasaToken(p, KW_PROGRAM);
    CasaToken(p, TK_ID);
    CasaToken(p, SMB_SEM);
    bloco(p);
    CasaToken(p, SMB_DOT);
}

/* <bloco> ::= <parte de declaracoes de variaveis> <comando composto> */
static void bloco(Parser *p)
{
    regra("<bloco> ::= <parte de declaracoes de variaveis> <comando composto>");
    parteDeclaracoes(p);
    comandoComposto(p);
}

/*
 * <parte de declaracoes de variaveis> ::=
 *     { var <declaracao de variaveis> { ; <declaracao de variaveis> } ; }
 */
static void parteDeclaracoes(Parser *p)
{
    regra("<parte de declaracoes de variaveis> ::= { var <declaracao de variaveis> { ; <declaracao de variaveis> } ; }");

    while (tipoCorrente(p) == KW_VAR) {
        CasaToken(p, KW_VAR);
        declaracaoVariaveis(p);
        CasaToken(p, SMB_SEM);

        /* Demais declaracoes do mesmo bloco var, cada uma seguida de ; */
        while (tipoCorrente(p) == TK_ID) {
            declaracaoVariaveis(p);
            CasaToken(p, SMB_SEM);
        }
    }
}

/* <declaracao de variaveis> ::= <lista de identificadores> : <tipo> */
static void declaracaoVariaveis(Parser *p)
{
    regra("<declaracao de variaveis> ::= <lista de identificadores> : <tipo>");
    listaIdentificadores(p);
    CasaToken(p, SMB_COL);
    tipo(p);
}

/* <lista de identificadores> ::= <identificador> { , <identificador> } */
static void listaIdentificadores(Parser *p)
{
    regra("<lista de identificadores> ::= <identificador> { , <identificador> }");
    CasaToken(p, TK_ID);

    while (tipoCorrente(p) == SMB_COM) {
        CasaToken(p, SMB_COM);
        CasaToken(p, TK_ID);
    }
}

/* <tipo> ::= integer | real */
static void tipo(Parser *p)
{
    if (tipoCorrente(p) == KW_INTEGER) {
        regra("<tipo> ::= integer");
        CasaToken(p, KW_INTEGER);
    } else if (tipoCorrente(p) == KW_REAL) {
        regra("<tipo> ::= real");
        CasaToken(p, KW_REAL);
    } else {
        erroSintatico(p);
    }
}

/* <comando composto> ::= begin <comando> ; { <comando> ; } end */
static void comandoComposto(Parser *p)
{
    regra("<comando composto> ::= begin <comando> ; { <comando> ; } end");
    CasaToken(p, KW_BEGIN);

    comando(p);
    CasaToken(p, SMB_SEM);

    while (ehInicioComando(p)) {
        comando(p);
        CasaToken(p, SMB_SEM);
    }

    CasaToken(p, KW_END);
}

/*
 * <comando> ::= <atribuicao> | <comando composto>
 *             | <comando condicional> | <comando repetitivo>
 *
 * A regra escolhida depende do token corrente.
 */
static void comando(Parser *p)
{
    switch (tipoCorrente(p)) {
        case TK_ID:
            regra("<comando> ::= <atribuicao>");
            atribuicao(p);
            break;
        case KW_BEGIN:
            regra("<comando> ::= <comando composto>");
            comandoComposto(p);
            break;
        case KW_IF:
            regra("<comando> ::= <comando condicional>");
            comandoCondicional(p);
            break;
        case KW_WHILE:
            regra("<comando> ::= <comando repetitivo>");
            comandoRepetitivo(p);
            break;
        default:
            erroSintatico(p);
    }
}

/* <atribuicao> ::= <variavel> := <expressao> */
static void atribuicao(Parser *p)
{
    regra("<atribuicao> ::= <variavel> := <expressao>");
    variavel(p);
    CasaToken(p, OP_ASS);
    expressao(p);
}

/* <comando condicional> ::= if <expressao> then <comando> [ else <comando> ] */
static void comandoCondicional(Parser *p)
{
    regra("<comando condicional> ::= if <expressao> then <comando> [ else <comando> ]");
    CasaToken(p, KW_IF);
    expressao(p);
    CasaToken(p, KW_THEN);
    comando(p);

    if (tipoCorrente(p) == KW_ELSE) {
        CasaToken(p, KW_ELSE);
        comando(p);
    }
}

/* <comando repetitivo> ::= while <expressao> do <comando> */
static void comandoRepetitivo(Parser *p)
{
    regra("<comando repetitivo> ::= while <expressao> do <comando>");
    CasaToken(p, KW_WHILE);
    expressao(p);
    CasaToken(p, KW_DO);
    comando(p);
}

/* <expressao> ::= <expressao simples> [ <relacao> <expressao simples> ] */
static void expressao(Parser *p)
{
    regra("<expressao> ::= <expressao simples> [ <relacao> <expressao simples> ]");
    expressaoSimples(p);

    if (ehRelacao(p)) {
        relacao(p);
        expressaoSimples(p);
    }
}

/* <relacao> ::= = | <> | < | <= | >= | > */
static void relacao(Parser *p)
{
    switch (tipoCorrente(p)) {
        case OP_EQ: regra("<relacao> ::= =");  CasaToken(p, OP_EQ); break;
        case OP_NE: regra("<relacao> ::= <>"); CasaToken(p, OP_NE); break;
        case OP_LT: regra("<relacao> ::= <");  CasaToken(p, OP_LT); break;
        case OP_LE: regra("<relacao> ::= <="); CasaToken(p, OP_LE); break;
        case OP_GE: regra("<relacao> ::= >="); CasaToken(p, OP_GE); break;
        case OP_GT: regra("<relacao> ::= >");  CasaToken(p, OP_GT); break;
        default:    erroSintatico(p);
    }
}

/* <expressao simples> ::= [ + | - ] <termo> { ( + | - ) <termo> } */
static void expressaoSimples(Parser *p)
{
    regra("<expressao simples> ::= [ + | - ] <termo> { ( + | - ) <termo> }");

    if (tipoCorrente(p) == OP_ADD)
        CasaToken(p, OP_ADD);
    else if (tipoCorrente(p) == OP_MIN)
        CasaToken(p, OP_MIN);

    termo(p);

    while (tipoCorrente(p) == OP_ADD || tipoCorrente(p) == OP_MIN) {
        if (tipoCorrente(p) == OP_ADD)
            CasaToken(p, OP_ADD);
        else
            CasaToken(p, OP_MIN);
        termo(p);
    }
}

/* <termo> ::= <fator> { ( * | / ) <fator> } */
static void termo(Parser *p)
{
    regra("<termo> ::= <fator> { ( * | / ) <fator> }");
    fator(p);

    while (tipoCorrente(p) == OP_MUL || tipoCorrente(p) == OP_DIV) {
        if (tipoCorrente(p) == OP_MUL)
            CasaToken(p, OP_MUL);
        else
            CasaToken(p, OP_DIV);
        fator(p);
    }
}

/* <fator> ::= <variavel> | <numero> | ( <expressao> ) */
static void fator(Parser *p)
{
    switch (tipoCorrente(p)) {
        case TK_ID:
            regra("<fator> ::= <variavel>");
            variavel(p);
            break;
        case TK_NUM_INT:
            regra("<fator> ::= <numero>");
            CasaToken(p, TK_NUM_INT);
            break;
        case TK_NUM_REAL:
            regra("<fator> ::= <numero>");
            CasaToken(p, TK_NUM_REAL);
            break;
        case SMB_OPA:
            regra("<fator> ::= ( <expressao> )");
            CasaToken(p, SMB_OPA);
            expressao(p);
            CasaToken(p, SMB_CPA);
            break;
        default:
            erroSintatico(p);
    }
}

/* <variavel> ::= <identificador> */
static void variavel(Parser *p)
{
    regra("<variavel> ::= <identificador>");
    CasaToken(p, TK_ID);
}

/* ------------------------------------------------------------------ */
/*  API publica                                                        */
/* ------------------------------------------------------------------ */
void analisar(const TokenVec *v)
{
    Parser p;
    p.toks = v->data;
    p.i    = 0;
    p.n    = v->size;

    /* O corpo principal chama o procedimento do simbolo inicial. */
    programa(&p);

    /* Depois do programa, so pode restar o fim de arquivo. */
    if (tipoCorrente(&p) != TK_EOF)
        erroSintatico(&p);

    printf("\nAnalise sintatica concluida com sucesso.\n");
}
