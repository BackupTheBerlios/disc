CC=gcc
YACC=bison
LEX=flex

all: interpret

lex.yy.c: cmd_proc.lex cmd_proc.h
	$(LEX) $<

y.tab.c: cmd_proc.y lex.yy.c cmd_proc.h list.h
	$(YACC) -v -y $<

yyerror.o: yyerror.c
	$(CC) -o $@ $<

interpret: y.tab.c yyerror.c interpret.c
	$(CC) -o $@ $^
	
clean:
	rm -f *.o	
	rm -f y.tab.c lex.yy.c y.output
	rm -f interpret
