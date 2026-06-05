# Relatorio Tecnico: Analisador Sintatico do MicroPascal

Disciplina: Linguagens Formais, Automatos e Compiladores (UCB, 1 semestre de 2026).

Grupo: Leonardo Silva de Alcantara e Marcelo Henrique.

## 1. Visao geral

O trabalho implementa o analisador sintatico da linguagem MicroPascal em C,
usando a tecnica de descida recursiva. O analisador recebe os tokens produzidos
pelo analisador lexico (Parte 01) e reconhece a estrutura do programa a partir
do simbolo inicial `<programa>`, imprimindo a sequencia de regras de producao
aplicadas. Quando o codigo nao segue a gramatica, o programa exibe a mensagem de
erro no formato exigido e encerra.

O fluxo geral e:

1. O lexico le o programa-fonte inteiro e guarda todos os tokens em um `TokenVec`.
2. O sintatico percorre esse vetor com um indice, sem voltar a chamar o lexico
   caractere a caractere.
3. A funcao `CasaToken` compara o token corrente com o token esperado pela regra.

## 2. Estruturas de dados

### 2.1 Token (lexer.h)
Representa uma unidade lexica. Campos:
- `type`: a categoria do token (palavra reservada, identificador, numero,
  operador ou simbolo), definida pelo enum `TokenType`.
- `lexeme`: o texto original do token, usado para imprimir `[lex]` nas mensagens
  de erro.
- `line`: a linha do programa-fonte onde o token aparece, usada para imprimir o
  numero `nn` nas mensagens de erro.
- `col`: a coluna (herdada da Parte 01, util para depuracao).

### 2.2 TokenVec (tokenvec.h)
Vetor dinamico que guarda todos os tokens do programa. E a ponte entre o lexico
e o sintatico. Campos:
- `data`: o arranjo de tokens.
- `size`: quantos tokens ja foram armazenados.
- `cap`: capacidade alocada; cresce dobrando quando o vetor enche.

### 2.3 Parser (sintatico.h)
Mantem o estado da analise sintatica. Campos:
- `toks`: ponteiro para o vetor de tokens (`TokenVec.data`).
- `i`: indice do token corrente (o "token corrente" citado no enunciado).
- `n`: quantidade total de tokens.

### 2.4 SymTable (symtable.h)
Tabela de simbolos por hash (reaproveitada da Parte 01). Guarda as palavras
reservadas pre-carregadas e os identificadores encontrados, permitindo ao lexico
distinguir uma palavra reservada de um identificador comum.

## 3. Funcoes

### 3.1 Lexico e vetor de tokens
- `getNextToken` (lexer.c): le e devolve o proximo token do arquivo-fonte.
- `tokenize_to_vector` (tokenvec.c): chama `getNextToken` repetidamente e
  empilha cada token no `TokenVec`, ate o token `TK_EOF`. Se o lexico achar um
  token invalido, exibe o erro lexico e encerra.
- `tv_free` (tokenvec.c): libera a memoria do vetor.

### 3.2 Apoio do parser (sintatico.c)
- `tipoCorrente`: devolve o tipo do token corrente.
- `regra`: imprime a regra de producao que esta sendo aplicada.
- `erroSintatico`: imprime a mensagem de erro no formato exigido e encerra.
  Distingue dois casos: fim de arquivo inesperado e token inesperado.
- `CasaToken`: compara o token corrente com o esperado. Se forem iguais, consome
  o token (avanca o indice); caso contrario, chama `erroSintatico`. Corresponde
  ao procedimento `CasaToken` pedido no enunciado.
- `ehInicioComando`: indica se o token corrente pode iniciar um comando.
- `ehRelacao`: indica se o token corrente e um operador relacional.

### 3.3 Procedimentos dos nao-terminais (sintatico.c)
Existe um procedimento para cada simbolo nao-terminal da gramatica. Cada um
imprime a regra aplicada e usa `CasaToken` para os terminais e chamadas aos
outros procedimentos para os nao-terminais:

- `programa`: `program <identificador> ; <bloco> .`
- `bloco`: parte de declaracoes seguida do comando composto.
- `parteDeclaracoes`: zero ou mais blocos `var ... ;`.
- `declaracaoVariaveis`: lista de identificadores, `:`, tipo.
- `listaIdentificadores`: um identificador seguido de zero ou mais `, id`.
- `tipo`: `integer` ou `real`.
- `comandoComposto`: `begin` comando `;` repeticoes `end`.
- `comando`: escolhe entre atribuicao, comando composto, condicional ou
  repetitivo conforme o token corrente.
- `atribuicao`: variavel, `:=`, expressao.
- `comandoCondicional`: `if` expressao `then` comando, com `else` opcional.
- `comandoRepetitivo`: `while` expressao `do` comando.
- `expressao`: expressao simples com relacao opcional.
- `relacao`: um dos operadores `=  <>  <  <=  >=  >`.
- `expressaoSimples`: sinal opcional, termo, e somas ou subtracoes.
- `termo`: fator, e multiplicacoes ou divisoes.
- `fator`: variavel, numero ou expressao entre parenteses.
- `variavel`: um identificador.

### 3.4 Ponto de entrada
- `analisar` (sintatico.c): inicializa o `Parser`, chama `programa` e, ao final,
  exige que so reste o token `TK_EOF`.
- `main` (main.c): le o arquivo indicado na linha de comando, gera o `TokenVec`
  e chama `analisar`.

## 4. Como compilar e executar

```
make
./sintatico testes/correto1.pas
```

Sem o make:

```
gcc -Wall -Wextra -std=c11 -O2 -o sintatico main.c tokenvec.c sintatico.c lexer.c symtable.c
```

## 5. Mensagens de erro

Conforme o enunciado, foram adotados os formatos (nn = linha, lex = lexema):

- `nn:token nao esperado [lex].`
- `nn:fim de arquivo nao esperado.`

Os acentos foram omitidos para evitar problemas de codificacao no terminal.

## 6. Testes realizados

### 6.1 Programas corretos

**testes/correto1.pas** (atribuicoes e expressoes aritmeticas)
```
program exemplo1;
var
  x, y : integer;
begin
  x := 10;
  y := x + 5 * 2;
  x := (y - 3) / 2;
end.
```
Saida: sequencia completa de regras a partir de `<programa>` e a mensagem
`Analise sintatica concluida com sucesso.`

**testes/correto2.pas** (comando condicional if/then/else)
```
program exemplo2;
var
  a, b, maior : integer;
begin
  a := 7;
  b := 12;
  if a > b then
    maior := a
  else
    maior := b;
end.
```
Saida: regras incluindo `<comando condicional>` e `<relacao> ::= >`, terminando
com sucesso.

**testes/correto3.pas** (comando repetitivo while com comando composto aninhado)
```
program exemplo3;
var
  i, soma : integer;
begin
  i := 1;
  soma := 0;
  while i <= 10 do
  begin
    soma := soma + i;
    i := i + 1;
  end;
end.
```
Saida: regras incluindo `<comando repetitivo>`, `<relacao> ::= <=` e um
`<comando composto>` aninhado, terminando com sucesso.

### 6.2 Programas com erro

**testes/erro1.pas** (falta de `;` apos um comando)
```
program erro1;
var
  x : integer;
begin
  x := 5
  x := x + 1;
end.
```
Saida: `6:token nao esperado [x].`
O `;` que deveria terminar `x := 5` esta ausente, entao ao tentar casar o `;` o
analisador encontra o identificador `x` da linha 6.

**testes/erro2.pas** (`:=` trocado por `=`)
```
program erro2;
var
  y : integer;
begin
  y = 10;
end.
```
Saida: `5:token nao esperado [=].`
Na atribuicao, apos a variavel `y` o analisador espera `:=` mas encontra `=`.

**testes/erro3.pas** (falta o `.` final)
```
program erro3;
var
  z : integer;
begin
  z := 1;
end
```
Saida: `7:fim de arquivo nao esperado.`
Apos o `end` o analisador espera o `.` que encerra o programa, mas encontra o fim
do arquivo.
