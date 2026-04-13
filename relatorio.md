# Relatório Técnico — Analisador Léxico MicroPascal

**Disciplina:** Linguagens Formais, Autômatos e Compiladores  
**Professor:** Marcelo Eustáquio  
**Instituição:** Universidade Católica de Brasília — 1° semestre de 2026

---

## 1. Descrição das Estruturas de Dados

### 1.1 `TokenType` (lexer.h)

Enumeração que classifica cada unidade léxica reconhecida pelo analisador:

| Categoria         | Valores                                                         |
|-------------------|-----------------------------------------------------------------|
| Palavras reservadas | `KW_PROGRAM`, `KW_VAR`, `KW_INTEGER`, `KW_REAL`, `KW_BEGIN`, `KW_END`, `KW_IF`, `KW_THEN`, `KW_ELSE`, `KW_WHILE`, `KW_DO` |
| Literais          | `TK_ID`, `TK_NUM_INT`, `TK_NUM_REAL`                           |
| Operadores        | `OP_EQ`, `OP_NE`, `OP_LT`, `OP_LE`, `OP_GT`, `OP_GE`, `OP_ADD`, `OP_MIN`, `OP_MUL`, `OP_DIV`, `OP_ASS` |
| Símbolos          | `SMB_SEM`, `SMB_COM`, `SMB_OPA`, `SMB_CPA`, `SMB_COL`, `SMB_DOT` |
| Especiais         | `TK_EOF`, `TK_ERROR`                                           |

### 1.2 `Token` (lexer.h)

Estrutura que representa um token reconhecido:

```c
typedef struct {
    TokenType type;        /* classificação do token       */
    char      lexeme[256]; /* texto original do lexema     */
    int       line;        /* linha no arquivo fonte       */
    int       col;         /* coluna no arquivo fonte      */
} Token;
```

### 1.3 `LexerState` (lexer.c — interno)

Estado interno do analisador léxico:

```c
struct LexerState {
    FILE     *src;     /* arquivo fonte aberto para leitura */
    int       line;    /* linha atual (começa em 1)         */
    int       col;     /* coluna atual (começa em 0)        */
    SymTable *symtab;  /* ponteiro para a tabela de símbolos */
};
```

### 1.4 `SymEntry` (symtable.h)

Nó de encadeamento na tabela de símbolos:

```c
typedef struct SymEntry {
    char             lexeme[256]; /* lexema em minúsculas    */
    TokenType        type;        /* tipo associado ao lexema */
    struct SymEntry *next;        /* próximo nó no bucket    */
} SymEntry;
```

### 1.5 `SymTable` (symtable.h)

Tabela de símbolos implementada como hash table com encadeamento externo:

```c
typedef struct SymTable {
    SymEntry *buckets[64]; /* vetor de 64 listas encadeadas */
} SymTable;
```

A função de hash utilizada é a **djb2-xor**:

```c
h = 5381;
h = ((h << 5) + h) ^ c;   /* para cada caractere c do lexema */
h = h % 64;
```

### 1.6 `NumState` (lexer.c — interno)

Enumeração dos estados do sub-AFD numérico:

| Estado   | Descrição                          | Estado final?       |
|----------|------------------------------------|---------------------|
| `N_INT`  | Lendo dígitos inteiros             | Sim → `TK_NUM_INT`  |
| `N_DOT`  | Viu `'.'`, aguarda dígito          | Não                 |
| `N_REAL` | Lendo parte decimal                | Sim → `TK_NUM_REAL` |
| `N_E`    | Viu `'E'`/`'e'`                    | Não                 |
| `N_ESIGN`| Viu sinal após `E`                 | Não                 |
| `N_EXP`  | Lendo dígitos do expoente          | Sim → `TK_NUM_REAL` |
| `N_DEAD` | Estado morto (erro)                | Não                 |

---

## 2. Descrição das Funções

### 2.1 `lexer.c`

#### `advance(LexerState *ls) → int`
Lê e retorna um caractere do arquivo fonte, atualizando `line` e `col`. Ao encontrar `'\n'`, incrementa `line` e zera `col`. Retorna `EOF` ao fim do arquivo.

#### `unadvance(LexerState *ls, int c)`
Devolve um caractere ao stream com `ungetc`, desfazendo a atualização de linha/coluna. Implementa o lookahead de 1 caractere exigido pelos operadores `<`, `>`, `:` e pelo ponto decimal em números reais.

#### `step_num(NumState s, int c) → NumState`
Função de transição do sub-AFD numérico. Recebe o estado atual e o caractere lido, retorna o próximo estado. Se não houver transição válida, retorna `N_DEAD`.

#### `num_is_final(NumState s) → int`
Retorna 1 se o estado numérico é final (`N_INT`, `N_REAL` ou `N_EXP`), indicando que o número reconhecido é válido.

#### `token_type_name(TokenType t) → const char *`
Converte um valor de `TokenType` para sua representação em string (ex.: `OP_ASS`, `KW_WHILE`). Usada na escrita do arquivo `.lex`.

#### `lexer_create(FILE *src, SymTable *symtab) → LexerState *`
Aloca e inicializa um `LexerState`. Define `line = 1` e `col = 0`. Associa o arquivo fonte e a tabela de símbolos ao estado.

#### `lexer_free(LexerState *ls)`
Libera a memória alocada para o `LexerState`. Não fecha o arquivo nem a tabela — ambos são gerenciados externamente.

#### `getNextToken(LexerState *ls) → Token`
Função principal do analisador léxico. A cada chamada avança no arquivo fonte e retorna o próximo `Token`. Implementa o AFD principal:

1. **Pula espaços em branco** — `advance` em loop até encontrar caractere não-espaço.
2. **Comentários `{ ... }`** — consome todos os caracteres até `}`. Se chegar ao `EOF` sem fechar, emite `TK_ERROR`.
3. **Identificadores / palavras reservadas** — ao encontrar letra, lê letras e dígitos (tudo em minúsculas via `tolower`). Consulta a tabela de símbolos com `symtable_lookup_or_insert`; o tipo retornado reflete se é `KW_*` ou `TK_ID`.
4. **Números** — ao encontrar dígito, aciona o sub-AFD numérico via `step_num`. Trata o caso especial do ponto decimal com lookahead duplo.
5. **Operadores com lookahead** — `<`, `>` e `:` leem o próximo caractere via `advance`; se não formar token de dois caracteres, o caractere é devolvido com `unadvance`.
6. **Tokens de um único caractere** — `=`, `+`, `-`, `*`, `/`, `;`, `,`, `(`, `)`, `.` são despachados diretamente.
7. **Caracteres inválidos** — qualquer outro caractere gera `TK_ERROR`.

### 2.2 `symtable.c`

#### `sym_hash(const char *s) → unsigned int`
Calcula o índice do bucket usando a função djb2-xor. Opera sobre lexemas em minúsculas, garantindo comportamento case-insensitive.

#### `symtable_create() → SymTable *`
Aloca e inicializa (com `calloc`) uma `SymTable` com todos os buckets nulos.

#### `symtable_free(SymTable *st)`
Percorre todos os buckets e libera cada nó `SymEntry` encadeado, depois libera a própria `SymTable`.

#### `symtable_lookup_or_insert(SymTable *st, const char *lexeme, TokenType type) → SymEntry *`
Busca `lexeme` na tabela via hash. Se encontrado, retorna a entrada existente sem alterar o tipo (garantindo que palavras reservadas não sejam sobrescritas por `TK_ID`). Se não encontrado, cria e insere um novo `SymEntry` na cabeça do bucket.

#### `symtable_load_keywords(SymTable *st)`
Pré-carrega as 11 palavras reservadas de MicroPascal na tabela, chamando `symtable_lookup_or_insert` para cada uma.

#### `symtable_print(SymTable *st, FILE *out)`
Percorre todos os buckets e imprime cada entrada no formato `Lexema   Tipo`, gerando o arquivo `.ts`.

### 2.3 `main.c`

#### `make_outname(const char *input, const char *ext, char *out, size_t outsz)`
Deriva o nome do arquivo de saída substituindo a extensão do arquivo de entrada. Ex.: `test/correto1.pas` + `".lex"` → `test/correto1.lex`.

#### `main(int argc, char *argv[]) → int`
Ponto de entrada. Fluxo de execução:

1. Valida argumentos e abre o arquivo fonte.
2. Deriva e abre os três arquivos de saída (`.lex`, `.ts`, `.err`).
3. Cria a `SymTable` e pré-carrega as palavras reservadas.
4. Cria o `LexerState`.
5. Loop: chama `getNextToken` até `TK_EOF`. Tokens válidos são escritos no `.lex`; erros são escritos no `.err`.
6. Ao final, imprime a tabela de símbolos no `.ts`.
7. Libera recursos e retorna 0 (sem erros) ou 1 (com erros léxicos).

---

## 3. Explicação do AFD

O AFD completo está descrito no arquivo `afd.dot` (diagrama Graphviz). A imagem gerada encontra-se em `graphviz.png`.

### Estados principais

| Estado     | Tipo       | Descrição                              |
|------------|------------|----------------------------------------|
| `S_START`  | Não-final  | Estado inicial; origem de todas as transições |
| `S_COMMENT`| Não-final  | Dentro de um comentário `{ ... }`      |
| `S_ID`     | **Final**  | Reconhecendo identificador ou palavra reservada |
| `N_INT`    | **Final**  | Número inteiro (`TK_NUM_INT`)          |
| `N_DOT`    | Não-final  | Após `'.'`, aguardando dígito          |
| `N_REAL`   | **Final**  | Número real com parte decimal (`TK_NUM_REAL`) |
| `N_E`      | Não-final  | Após `'E'`/`'e'` em notação científica |
| `N_ESIGN`  | Não-final  | Após sinal do expoente (`+` ou `-`)    |
| `N_EXP`    | **Final**  | Expoente reconhecido (`TK_NUM_REAL`)   |
| `S_LT`     | Não-final  | Após `<`, aguardando `=` ou `>`        |
| `S_GT`     | Não-final  | Após `>`, aguardando `=`               |
| `S_COL`    | Não-final  | Após `:`, aguardando `=`               |
| `ERR`      | Erro       | Caractere inválido ou comentário não fechado |

### Transições de lookahead

O AFD emprega lookahead de 1 caractere em três situações:

- **`<`** → lê próximo: `=` → `OP_LE`; `>` → `OP_NE`; outro → putback → `OP_LT`
- **`>`** → lê próximo: `=` → `OP_GE`; outro → putback → `OP_GT`
- **`:`** → lê próximo: `=` → `OP_ASS`; outro → putback → `SMB_COL`
- **Ponto decimal** — ao ler `'.'` em estado `N_INT`, lê mais um caractere: se dígito → transita para `N_DOT` (número real); caso contrário → dois `unadvance` → encerra inteiro e devolve `'.'` ao stream.

---

## 4. Testes Realizados

### 4.1 Programas corretos

#### `correto1.pas` — Programa completo do enunciado

```pascal
{ Exemplo do enunciado — programa completo }
program Exemplo;

var
   x, y : integer;
   z    : real;

begin
   x := 10;
   y := 20;
   z := x + y * 2.5;

   if x > y then
      x := x - 1
   else
      y := y + 1;

   while z <= 100 do
   begin
      z := z * 1.5;
      x := x + 2
   end
end.
```

**Saída `.lex` (primeiras e últimas linhas):**
```
<KW_PROGRAM, program> 2 1
<TK_ID, exemplo> 2 9
<SMB_SEM, ;> 2 16
...
<KW_END, end> 23 1
<SMB_DOT, .> 23 4
```

**Arquivo `.err`:** vazio (nenhum erro léxico).

---

#### `correto2.pas` — Números reais e notação científica

```pascal
{ Teste de numeros reais e notacao cientifica }
program Numeros;

var
   a, b, c : real;

begin
   a := 3.14;
   b := 1.5E+2;
   c := 2.0E10;
   a := b + c * 0.001
end.
```

**Tokens numéricos reconhecidos:**
```
<TK_NUM_REAL, 3.14> 8 9
<TK_NUM_REAL, 1.5E+2> 9 9
<TK_NUM_REAL, 2.0E10> 10 9
<TK_NUM_REAL, 0.001> 11 17
```

**Arquivo `.err`:** vazio.

---

#### `correto3.pas` — Operadores relacionais e estruturas aninhadas

```pascal
{ Teste de operadores relacionais e estruturas aninhadas }
program Relacional;

var
   i, j, k : integer;

begin
   i := 1; j := 2; k := 3;

   if i <> j then
      if i <= k then
         k := k - i
      else
         k := k + j;

   while i < j do
   begin
      if j >= k then
         j := j - 1;
      i := i + 1
   end
end.
```

**Saída `.lex` completa:**
```
<KW_PROGRAM, program> 2 1
<TK_ID, relacional> 2 9
<SMB_SEM, ;> 2 19
<KW_VAR, var> 4 1
<TK_ID, i> 5 4
<SMB_COM, ,> 5 5
<TK_ID, j> 5 7
<SMB_COM, ,> 5 8
<TK_ID, k> 5 10
<SMB_COL, :> 5 12
<KW_INTEGER, integer> 5 14
<SMB_SEM, ;> 5 21
<KW_BEGIN, begin> 7 1
<TK_ID, i> 8 4
<OP_ASS, :=> 8 6
<TK_NUM_INT, 1> 8 9
<SMB_SEM, ;> 8 10
<TK_ID, j> 9 4
<OP_ASS, :=> 9 6
<TK_NUM_INT, 2> 9 9
<SMB_SEM, ;> 9 10
<TK_ID, k> 10 4
<OP_ASS, :=> 10 6
<TK_NUM_INT, 3> 10 9
<SMB_SEM, ;> 10 10
<KW_IF, if> 12 4
<TK_ID, i> 12 7
<OP_NE, <>> 12 9
<TK_ID, j> 12 12
<KW_THEN, then> 12 14
<KW_IF, if> 13 7
<TK_ID, i> 13 10
<OP_LE, <=> 13 12
<TK_ID, k> 13 15
<KW_THEN, then> 13 17
<TK_ID, k> 14 10
<OP_ASS, :=> 14 12
<TK_ID, k> 14 15
<OP_MIN, -> 14 17
<TK_ID, i> 14 19
<KW_ELSE, else> 15 7
<TK_ID, k> 16 10
<OP_ASS, :=> 16 12
<TK_ID, k> 16 15
<OP_ADD, +> 16 17
<TK_ID, j> 16 19
<SMB_SEM, ;> 16 20
<KW_WHILE, while> 18 4
<TK_ID, i> 18 10
<OP_LT, <> 18 12
<TK_ID, j> 18 14
<KW_DO, do> 18 16
<KW_BEGIN, begin> 19 4
<KW_IF, if> 20 7
<TK_ID, j> 20 10
<OP_GE, >=> 20 12
<TK_ID, k> 20 15
<KW_THEN, then> 20 17
<TK_ID, j> 21 10
<OP_ASS, :=> 21 12
<TK_ID, j> 21 15
<OP_MIN, -> 21 17
<TK_NUM_INT, 1> 21 19
<SMB_SEM, ;> 21 20
<TK_ID, i> 22 7
<OP_ASS, :=> 22 9
<TK_ID, i> 22 12
<OP_ADD, +> 22 14
<TK_NUM_INT, 1> 22 16
<KW_END, end> 23 4
<KW_END, end> 24 1
<SMB_DOT, .> 24 4
```

**Arquivo `.err`:** vazio.

---

### 4.2 Programas com erros léxicos

#### `erro1.pas` — Caractere inválido `@`

```pascal
{ Erro: caractere invalido @ }
program Teste;
var
   x : integer;
begin
   x := 10 @ 2
end.
```

**Arquivo `.err`:**
```
Linha 8, Coluna 12: caractere invalido '@'
```

---

#### `erro2.pas` — Comentário não fechado antes do EOF

```pascal
{ Comentario fechado normalmente }
program Teste;
var
   x : integer;
begin
   x := 5
   { este comentario nao foi fechado antes do EOF
end.
```

**Arquivo `.err`:**
```
Linha 9, Coluna 4: comentario nao fechado
```

---

#### `erro3.pas` — Múltiplos caracteres inválidos (`%` e `$`)

```pascal
{ Erro: caracteres invalidos % e $ }
program Teste;
var
   x : integer;
begin
   x := 5 % 3;
   x := x $ 1
end.
```

**Arquivo `.err`:**
```
Linha 8, Coluna 11: caractere invalido '%'
Linha 9, Coluna 11: caractere invalido '$'
```

O analisador detecta ambos os erros e continua o reconhecimento após cada um.

---

## 5. Conteúdo Final da Tabela de Símbolos

A ordem de exibição é determinada pelos índices dos buckets da hash table (não é ordenada alfabeticamente). Cada lexema aparece uma única vez, independentemente de quantas vezes ocorre no programa fonte.

### `correto1.pas` — identificadores: `exemplo`, `x`, `y`, `z`

```
Lexema               Tipo
------               ----
begin                KW_BEGIN
integer              KW_INTEGER
end                  KW_END
do                   KW_DO
program              KW_PROGRAM
while                KW_WHILE
else                 KW_ELSE
y                    TK_ID
x                    TK_ID
z                    TK_ID
var                  KW_VAR
exemplo              TK_ID
if                   KW_IF
then                 KW_THEN
real                 KW_REAL
```

### `correto2.pas` — identificadores: `numeros`, `a`, `b`, `c`

```
Lexema               Tipo
------               ----
begin                KW_BEGIN
integer              KW_INTEGER
a                    TK_ID
c                    TK_ID
b                    TK_ID
end                  KW_END
do                   KW_DO
program              KW_PROGRAM
while                KW_WHILE
else                 KW_ELSE
var                  KW_VAR
if                   KW_IF
then                 KW_THEN
numeros              TK_ID
real                 KW_REAL
```

### `correto3.pas` — identificadores: `relacional`, `i`, `j`, `k`

```
Lexema               Tipo
------               ----
begin                KW_BEGIN
integer              KW_INTEGER
end                  KW_END
i                    TK_ID
k                    TK_ID
do                   KW_DO
j                    TK_ID
program              KW_PROGRAM
while                KW_WHILE
else                 KW_ELSE
var                  KW_VAR
if                   KW_IF
then                 KW_THEN
relacional           TK_ID
real                 KW_REAL
```
