CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g
SRCS    = src/main.c src/lexer.c src/symtable.c
TARGET  = lexer

$(TARGET): $(SRCS) src/lexer.h src/symtable.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) test/*.lex test/*.ts test/*.err

run: $(TARGET)
	./$(TARGET) test/correto1.pas

test: $(TARGET)
	@echo "=== Testes corretos ==="
	./$(TARGET) test/correto1.pas
	./$(TARGET) test/correto2.pas
	./$(TARGET) test/correto3.pas
	@echo ""
	@echo "=== Testes com erros (esperado codigo de saida 1) ==="
	./$(TARGET) test/erro1.pas  || true
	./$(TARGET) test/erro2.pas  || true
	./$(TARGET) test/erro3.pas  || true
