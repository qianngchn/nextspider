PLAT:=none
PLATS:=linux mingw

CC:=gcc -std=gnu99
CFLAGS:=-Wall -O2 $(PLATCFLAGS)
LDFLAGS:=-lpthread -llua -lcurl $(PLATLDFLAGS)
SRCS:=main.c
OBJS:=$(SRCS:.c=.o)

BIN:=nextspider

ALL:=$(BIN)

INSTALL_TOP?=$(CURDIR)/local
INSTALL_BIN:=$(INSTALL_TOP)/bin

INSTALL:=install -p
INSTALL_EXEC:=$(INSTALL) -m0755

AR:=ar -rcu
RANLIB:=ranlib
STRIP:=strip -s
MKDIR:=mkdir -p
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
	$(CC) -c $^ $(CFLAGS)

$(BIN):$(OBJS)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@

install:
	$(MKDIR) $(INSTALL_BIN)
	$(INSTALL_EXEC) $(BIN) $(INSTALL_BIN)

uninstall:
	cd $(INSTALL_BIN) && $(RM) $(BIN)

clean:
	$(RM) $(OBJS)
	$(RM) $(ALL)
