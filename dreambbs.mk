## Common BSD make rules for DreamBBS Project

ARCHI	!= getconf LONG_BIT

OPSYS	!= uname -o

BUILDTIME	!= date '+%s'

BBSHOME	?= $(HOME)


## Toolchain settings

CC	= clang

RANLIB	= ranlib

CPROTO	= cproto -E"$(CC) -pipe -E" -I$(SRCROOT)/include

CFLAGS	= -ggdb3 -O0 -pipe -fomit-frame-pointer -Wall -Wno-invalid-source-encoding -I$(SRCROOT)/include

LDFLAGS	= -L$(SRCROOT)/lib -ldao -lcrypt

## Tool functions
## Called with $(function$(para1::=arg1)$(para2::=arg2)...)
GETCONF = echo "" | $(CC) -x c -dM -E -P $(hdr:@v@-imacros "$v"@) - | grep -wq $(conf)

## BBS Release Version Prefix
BBSCONF_ORIGIN		:= $(SRCROOT)/include/config.h
BBSVER != grep BBSVER_PREFIX ${BBSCONF_ORIGIN} | awk 'NR==1 {printf $$3}'

# rules ref: PttBBS: mbbsd/Makefile
BBSCONF		:= $(SRCROOT)/dreambbs.conf
DEF_YES		:= && echo "YES" || echo ""
USE_PMORE	!= sh -c '${GETCONF${conf::= "M3_USE_PMORE"}${hdr::= ${BBSCONF}}} ${DEF_YES}'
USE_PFTERM	!= sh -c '${GETCONF${conf::= "M3_USE_PFTERM"}${hdr::= ${BBSCONF}}} ${DEF_YES}'
USE_BBSLUA	!= sh -c '${GETCONF${conf::= "M3_USE_BBSLUA"}${hdr::= ${BBSCONF}}} ${DEF_YES}'
USE_BBSRUBY	!= sh -c '${GETCONF${conf::= "M3_USE_BBSRUBY"}${hdr::= ${BBSCONF}}} ${DEF_YES}'
USE_LUAJIT	!= sh -c '${GETCONF${conf::= "BBSLUA_USE_LUAJIT"}${hdr::= ${BBSCONF}}} ${DEF_YES}'

CLANG_MODERN != [ "$(CC)" = "clang" -a $$(echo '__clang_major__' | $(CC) -E - | tail -1) -ge 6 ] ${DEF_YES}

.if $(ARCHI)=="64"
CFLAGS	+= -m32
LDFLAGS	+= -m32
.endif

.if $(OPSYS) == "GNU/Linux"
LDFLAGS	+= -lresolv -ldl -rdynamic
.endif

.if $(OPSYS) == "FreeBSD"
LDFLAGS	+= -Wl,-export-dynamic
.endif

.if $(CLANG_MODERN)
CFLAGS  += -Wunreachable-code-aggressive
.else
CFLAGS  += -Wunreachable-code
.endif

# BBS-Lua & BBS-Ruby make rule definitions

.if $(USE_BBSLUA)
.if $(OPSYS) == "FreeBSD"
    LUA_LDFLAGS_ARCHI	= -Wl,--no-as-needed
.endif

.if $(USE_LUAJIT)
    LUA_PKG_NAME	?= luajit
.else
.if $(OPSYS) == "FreeBSD"
        LUA_PKG_NAME	?= lua-5.1
.else
        LUA_PKG_NAME	?= lua5.1
.endif
.endif

LUA_CFLAGS	!= pkg-config --cflags ${LUA_PKG_NAME}
LUA_LDFLAGS	!= pkg-config --libs ${LUA_PKG_NAME}
.endif


.if $(USE_BBSRUBY)
.if $(OPSYS) == "FreeBSD"
    RUBY_LDFLAGS_ARCHI	= -Wl,--no-as-needed
.endif
.if $(ARCHI) == "64"
    RUBY_CFLAGS_CMD	= | sed 's/x86_64/i386/'
.endif

RUBY_CFLAGS	!= pkg-config --cflags ruby-2.2 ${RUBY_CFLAGS_CMD}
RUBY_LDFLAGS	!= pkg-config --libs ruby-2.2
.endif
