#
# CS252 - Shell Project
#
#Use GNU compiler
cc= gcc
CC= g++
ccFLAGS= -g -std=c11
CCFLAGS= -g -std=c++17
WARNFLAGS= -Wall -Wextra -pedantic

LEX=lex -l
YACC=yacc -y -d -t --debug

EDIT_MODE_ON=yes

ifdef EDIT_MODE_ON
	EDIT_MODE_OBJECTS=tty-raw-mode.o read-line.o
endif

#all: git-commit shell
all: shell

lex.yy.o: shell.l 
	$(LEX) -o lex.yy.cc shell.l
	$(CC) $(CCFLAGS) -c lex.yy.cc

y.tab.o: shell.y
	$(YACC) -o y.tab.cc shell.y
	$(CC) $(CCFLAGS) -c y.tab.cc

regular.o: regular.c
	$(cc) $(ccFLAGS) $(WARNFLAGS) -c regular.c

gethomedir.o: gethomedir.cc
	$(CC) $(CCFLAGS) $(WARNFLAGS) -c gethomedir.cc

command.o: command.cc command.h
	$(CC) $(CCFLAGS) $(WARNFLAGS) -c command.cc

tty-raw-mode.o: tty-raw-mode.c
	$(cc) $(ccFLAGS) $(WARNFLAGS) -c tty-raw-mode.c

read-line.o: read-line.c
	$(cc) $(ccFLAGS) $(WARNFLAGS) -c read-line.c

shell: y.tab.o lex.yy.o gethomedir.o regular.o command.o $(EDIT_MODE_OBJECTS)
		$(CC) $(CCFLAGS) $(WARNFLAGS) -o shell lex.yy.o y.tab.o gethomedir.o regular.o command.o $(EDIT_MODE_OBJECTS) -lfl

.PHONY: git-commit
git-commit:
	git checkout master >> .local.git.out || echo
	git add *.cc *.hh *.l *.y Makefile >> .local.git.out  || echo
	git add test-shell/testall.out >> .local.git.out  || echo
	git commit -a -m 'Commit' >> .local.git.out || echo
	git push origin master

.PHONY: clean
clean:
	rm -f lex.yy.cc y.tab.cc y.tab.hh shell *.o
	rm -f test-shell/out test-shell/out2
	rm -f test-shell/sh-in test-shell/sh-out
	rm -f test-shell/shell-in test-shell/shell-out
	rm -f test-shell/err1 test-shell/file-list

