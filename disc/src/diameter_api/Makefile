# $Id: Makefile,v 1.1 2003/03/07 10:34:24 bogdan Exp $
#
#


#set some vars from the environment (and not make builtins)
CC := $(shell echo "$${CC}")


#defines
ALLDEP=Makefile
BIN=main
CFLAGS=
DEFS=
LIBS= -lpthread -lresolv
LDFLAGS=


#checks
ifeq ($(CC),)
	CC=gcc
endif


#more defines
LD= $(CC)
MKDEP=$(CC) -MM

#sources
sources=$(filter-out $(auto_gen), $(wildcard *.c) $(wildcard utils/*.c) \
		$(wildcard tcp_shell/*.c) ) $(auto_gen)
objs=$(sources:.c=.o)
depends=$(sources:.c=.d)


#compiling mode
#mode = debug
ifeq ($(mode),)
	mode = release
endif


#CFLAGS and LDFLAGS
ifeq ($(mode), release)
	CFLAGS=-g -O9 -funroll-loops  -Wcast-align $(PROFILE) -Wall
	LDFLAGS=-Wl,-O2 -Wl,-E
else
	CFLAGS=-g -Wcast-align
	LDFLAGS+=-g -Wl,-E
endif


#******************************** RULES ***************************************


#implicit rules
%.o:%.c  $(ALLDEP)
	$(CC) $(CFLAGS) $(DEFS) -c $< -o $@

%.d: %.c $(ALLDEP)
	@set -e; $(MKDEP) $(DEFS) $< \
	|  sed 's#\($*\)\.o[ :]*#\1.o $@ : #g' > $@; \
	[ -s $@ ] || rm -f $@


#common rules

.PHONY: all
all: $(BIN)

.PHONY: dep
dep: $(depends)

.PHONY: bin
$(BIN):  $(objs) $(ALLDEP)
	$(LD) $(LDFLAGS) $(objs) $(LIBS) -o $(BIN)


.PHONY: TAGS
TAGS:
	ctags-exuberant -R .


.PHONY: dist
dist:
	cd ..; tar czf main.tar.gz main/main.c main/Makefile


.PHONY: clean
clean:
	@rm -fr $(BIN)
	@rm -f $(objs) $(NAME) $(objs:.o=.il) 2>/dev/null


.PHONY: proper
proper: clean
	-@rm -f $(depends) $(auto_gen) 2>/dev/null


ifeq (,$(MAKECMDGOALS))
include $(depends)
endif

