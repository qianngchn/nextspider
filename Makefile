CC=gcc
CFLAGS=-Wall -O2 -lpthread -lcurl -llua -ldl -lm
SRCS=main.c

ETC=config.lua default.lua
BIN=nextspider

INSTALL_TOP=$(HOME)
INSTALL_ETC=$(HOME)/.$(BIN)
INSTALL_BIN=$(INSTALL_TOP)/bin

INSTALL=install -p
INSTALL_DATA=$(INSTALL) -m0644
INSTALL_EXEC=$(INSTALL) -m0755

MKDIR=mkdir -p
CP=cp -a
RM=rm -rf

.PHONY:all install uninstall clean

all:$(SRCS)
	$(CC) $(SRCS) $(CFLAGS) -o $(BIN)

install:
	$(MKDIR) $(INSTALL_ETC) $(INSTALL_BIN)
	$(INSTALL_DATA) $(ETC) $(INSTALL_ETC)
	$(INSTALL_EXEC) $(BIN) $(INSTALL_BIN)

uninstall:
	cd $(INSTALL_BIN) && $(RM) $(BIN)

clean:
	$(RM) $(BIN)
