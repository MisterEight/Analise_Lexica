# Analisador Léxico — MicroPascal

Trabalho prático da disciplina **Linguagens Formais, Autômatos e Compiladores** (UCB, 2026-1).

Implementa um analisador léxico para a linguagem MicroPascal usando um Autômato Finito Determinístico (AFD) em C11, sem ferramentas geradoras (flex, etc.).

---

## Estrutura do projeto

```
.
├── src/
│   ├── main.c        -- ponto de entrada, loop de tokens, escrita dos arquivos de saída
│   ├── lexer.h       -- TokenType, Token, API pública do lexer
│   ├── lexer.c       -- AFD, getNextToken(), advance/unadvance
│   ├── symtable.h    -- API da tabela de símbolos
│   └── symtable.c    -- hash table, pré-carga de palavras reservadas
├── test/
│   ├── correto1.pas  -- programa válido (exemplo do enunciado)
│   ├── correto2.pas  -- reais e notação científica
│   ├── correto3.pas  -- if/while aninhados e operadores relacionais
│   ├── erro1.pas     -- caractere inválido (@)
│   ├── erro2.pas     -- comentário não fechado
│   └── erro3.pas     -- caracteres inválidos (% e $)
├── afd.dot           -- diagrama Graphviz do AFD completo
├── plano.md          -- plano de implementação
├── Makefile
└── relatorio.md      -- relatório técnico (a entregar)
```

---

## Como compilar

```bash
gcc -Wall -Wextra -std=c11 -g -o lexer src/main.c src/lexer.c src/symtable.c
```

Ou, se o `make` estiver disponível:

```bash
make
```

---

## Como usar

```bash
./lexer <arquivo.pas>
```

**Exemplo:**

```bash
./lexer test/correto1.pas
```

Gera três arquivos de saída a partir do nome do arquivo fonte:

| Arquivo | Conteúdo |
|---|---|
| `<nome>.lex` | Lista de tokens reconhecidos |
| `<nome>.ts` | Tabela de símbolos (identificadores e palavras reservadas) |
| `<nome>.err` | Erros léxicos encontrados (linha e coluna) |

---

## Formato de saída

**.lex**
```
<KW_PROGRAM, program> 1 1
<TK_ID, exemplo> 1 9
<OP_ASS, :=> 2 4
<TK_NUM_INT, 10> 2 7
```

**.ts**
```
Lexema               Tipo
------               ----
program              KW_PROGRAM
x                    TK_ID
...
```

**.err**
```
Linha 5, Coluna 12: caractere invalido '@'
Linha 9, Coluna 4: comentario nao fechado
```

---

## Tokens da linguagem

| Token | Lexema |
|---|---|
| `KW_PROGRAM` … `KW_DO` | palavras reservadas |
| `TK_ID` | identificadores |
| `TK_NUM_INT` | inteiros: `42`, `100` |
| `TK_NUM_REAL` | reais: `3.14`, `1.5E+2`, `2.0E10` |
| `OP_ASS` | `:=` |
| `OP_EQ/NE/LT/LE/GT/GE` | `=` `<>` `<` `<=` `>` `>=` |
| `OP_ADD/MIN/MUL/DIV` | `+` `-` `*` `/` |
| `SMB_SEM/COM/OPA/CPA/COL/DOT` | `;` `,` `(` `)` `:` `.` |

Comentários entre `{ }` são ignorados. A linguagem é **case-insensitive**.

---

## Executar todos os testes

```bash
make test
```

Ou manualmente:

```bash
for f in test/*.pas; do echo "=== $f ==="; ./lexer "$f"; done
```

---

## Diagrama do AFD

O arquivo `afd.dot` contém o diagrama completo. Para gerar a imagem (requer Graphviz):

```bash
dot -Tpng afd.dot -o afd.png
```
