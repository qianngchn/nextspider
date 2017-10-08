PLAT:=none
PLATS:=linux mingw

CC:=gcc -std=gnu99
CFLAGS:=-Wall -O2 -s $(PLATCFLAGS)
LDFLAGS:=-lpthread -llua -lcurl $(PLATLDFLAGS)
SRCS:=main.c
OBJS:=$(SRCS:.c=.o)

BIN:=nextspider
ETC:=
INC:=
SHAREDLIB:=
STATICLIB:=

ALL:=$(BIN) $(SHAREDLIB) $(STATICLIB)

INSTALL_TOP?=$(HOME)/local
INSTALL_BIN:=$(INSTALL_TOP)/bin
INSTALL_ETC:=$(INSTALL_TOP)/etc
INSTALL_INC:=$(INSTALL_TOP)/include
INSTALL_LIB:=$(INSTALL_TOP)/lib

INSTALL:=install -p
INSTALL_EXEC:=$(INSTALL) -m0755
INSTALL_DATA:=$(INSTALL) -m0644

AR:=ar -rcu
RANLIB:=ranlib
STRIP:=strip -s
MKDIR:=mkdir -p
CP:=cp -a
RM:=rm -rf

.PHONY:all none $(PLATS) install uninstall clean

all:$(PLAT)

none:
	@echo "Please select a PLATFORM from these:"
	@echo "    $(PLATS)"
	@echo "then do 'make PLATFORM' to complete instructions."

linux:
	$(MAKE) $(ALL) "PLATLDFLAGS=-ldl -lm"

mingw:
	$(MAKE) $(ALL) "PLATCFLAGS=-static -DCURL_STATICLIB" "PLATLDFLAGS=-lssl -lcrypto -lgdi32 -lwldap32 -lz -lws2_32"

$(OBJS):$(SRCS)
	$(CC) -c $^ $(CFLAGS) $(LDFLAGS)

$(BIN):$(OBJS)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@

$(SHAREDLIB):$(OBJS)
	$(CC) $^ -shared -fPIC $(CFLAGS) $(LDFLAGS) -o $@
	$(STRIP) $@

$(STATICLIB):$(OBJS)
	$(AR) $@ $^
	$(RANLIB) $@

install:
	$(MKDIR) $(INSTALL_BIN)
	$(INSTALL_EXEC) $(BIN) $(INSTALL_BIN)

uninstall:
	cd $(INSTALL_BIN) && $(RM) $(BIN)

clean:
	$(RM) $(OBJS)
	$(RM) $(ALL)
