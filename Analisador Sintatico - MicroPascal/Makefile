CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2
OBJ     = main.o tokenvec.o sintatico.o lexer.o symtable.o
ALVO    = sintatico

$(ALVO): $(OBJ)
	$(CC) $(CFLAGS) -o $(ALVO) $(OBJ)

main.o: main.c tokenvec.h sintatico.h
tokenvec.o: tokenvec.c tokenvec.h lexer.h symtable.h
sintatico.o: sintatico.c sintatico.h tokenvec.h lexer.h
lexer.o: lexer.c lexer.h symtable.h
symtable.o: symtable.c symtable.h lexer.h

clean:
	rm -f $(OBJ) $(ALVO)
