SHELL := /bin/bash


# ==================================================
# COMMANDS

CC = ~/cross-pi-gcc-9.1.0-0/bin/arm-linux-gnueabihf-gcc
#CC = gcc
RM = rm -f

# ==================================================
# DIRECTORIES

SRC = src
BIN = bin

# ==================================================
# TARGETS

EXEC = espx


# ==================================================
# COMPILATION

all: $(EXEC)

# -- add any dependencies here
%: $(SRC)/%.c
	$(CC) $< -o $(BIN)/$@ -lpthread

clean:
	$(RM) $(SRC)/*~ *~

purge: clean
	$(RM) $(addprefix $(BIN)/, $(EXEC))
