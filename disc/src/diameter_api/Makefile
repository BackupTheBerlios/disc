# $Id: Makefile,v 1.2 2003/03/12 18:12:22 andrei Exp $
#
#
# 2003-03-12  added SHM_MEM & other defines from ser;
#              version number, arhitecture, etc. (andrei)


#set some vars from the environment (and not make builtins)
CC := $(shell echo "$${CC}")

NAME=diameter_api
#version number
VERSION = 0
PATCHLEVEL = 0
SUBLEVEL =   1
EXTRAVERSION = pre1

RELEASE=$(VERSION).$(PATCHLEVEL).$(SUBLEVEL)$(EXTRAVERSION)
OS = $(shell uname -s | sed -e s/SunOS/solaris/ | tr "[A-Z]" "[a-z]")
ARCH = $(shell uname -m |sed -e s/i.86/i386/ -e s/sun4u/sparc64/  \
			-e s/armv4l/arm/)

#defines
ALLDEP=Makefile
BIN=main
CFLAGS=
LIBS= -lpthread -lresolv
LDFLAGS=



# compile-time options (from ser Makefile.defs)
#
# -DNO_DEBUG
#		turns off some of the debug messages (DBG(...)).
# -DNO_LOG
#		completely turns of all the logging (and DBG(...))
# -DDNS_IP_HACK
#		faster ip address resolver for ip strings (e.g "127.0.0.1")
# -DSHM_MEM
#		compiles in shared mem. support, needed by some modules and
#		by USE_SHM_MEM
# -DSHM_MMAP
#		use mmap instead of SYSV shared memory
# -DPKG_MALLOC
#		uses a faster malloc (exclusive w/ USE_SHM_MEM)
# -DUSE_SHM_MEM
#		all pkg_malloc => shm_malloc (most mallocs use a common sh.
#		mem. segment); don't define PKG_MALLOC if you want this!
# -DDBG_QM_MALLOC
#		qm_malloc debug code, will cause pkg_malloc and shm_malloc
#		to keep and display lot of debuging information: file name,
#		function, line number of malloc/free call for each block,
#		extra error checking (trying to free the same pointer
#		twice, trying to free a pointer alloc'ed with a different
#		malloc etc.)
# -DVQ_MALLOC
#		additional option to PKG_MALLOC which utilizes a fater then
#		qm version
#		(not true anymore, q_malloc performs approx. the same)
# -DF_MALLOC
#		an even faster malloc, not recommended for debugging
# -DDBG_MALLOC
#		issues additional debugging information if lock/unlock is called
# -DFAST_LOCK
#		uses fast arhitecture specific locking (see the arh. specific section)
# -DUSE_SYSV_SEM
#		uses sys v sems for locking (slower & limited number)
# -DUSE_PTHREAD_MUTEX
#		uses pthread mutexes, faster than sys v or posix sems, but do not 
#		work on all systems inter-processes (e.g. linux)
# -DUSE_POSIX_SEM
#		uses posix semaphores for locking (faster than sys v)
# -DBUSY_WAIT
#		uses busy waiting on the lock
# -DADAPTIVE_WAIT
#		try busy waiting for a while and if the lock is still held go to
#		force reschedule
# -DADAPTIVE_WAIT_LOOPS=number
#		number of loops we busy wait, after "number" loops have elapsed we 
#		force a reschedule
# -DNOSMP
#		don't use smp compliant locking (faster but won't work on SMP machines)
#		(not yet enabled)


DEFS+= -DNAME='"$(NAME)"' -DVERSION='"$(RELEASE)"' -DARCH='"$(ARCH)"' \
	 -DOS='"$(OS)"' -DCOMPILER='"$(CC_VER)"' -D__CPU_$(ARCH)\
	 -DSHM_MEM \
	 -DSHM_MMAP \
	 -DDBG_QM_MALLOC \
	 -DADAPTIVE_WAIT -DADAPTIVE_WAIT_LOOPS=1024 \
	 -DFAST_LOCK \
	 #-DDNS_IP_HACK \
	 #-DF_MALLOC \
	 #-DNO_DEBUG \
	 #-DNO_LOG   \
	 #-DUSE_SHM_MEM \
	 #-DPKG_MALLOC \
	 #-DVQ_MALLOC  
	 #-DNOSMP \

# linux specific HAVE*
	DEFS+=-DHAVE_GETHOSTBYNAME2 -DHAVE_UNION_SEMUN -DHAVE_SCHED_YIELD \
			-DHAVE_MSG_NOSIGNAL -DHAVE_MSGHDR_MSG_CONTROL

#checks
ifeq ($(CC),)
	CC=gcc
endif


#more defines
LD= $(CC)
MKDEP=$(CC) -MM

#sources
sources=$(filter-out $(auto_gen), $(wildcard *.c) $(wildcard utils/*.c) \
		$(wildcard tcp_shell/*.c) $(wildcard mem/*.c) ) $(auto_gen)
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

