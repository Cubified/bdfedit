all: bdfedit

CC=cc

LIBS=
CFLAGS=-O3 -pipe
DEBUGCFLAGS=-Og -pipe -g

INPUT=bdfedit.c lib/vec.c lib/bdf.c
OUTPUT=bdfedit

RM=/bin/rm

.PHONY: bdfedit
bdfedit:
	$(CC) $(INPUT) -o $(OUTPUT) $(LIBS) $(CFLAGS)
debug:
	$(CC) $(INPUT) -o $(OUTPUT) $(LIBS) $(DEBUGCFLAGS)
clean:
	if [ -e $(OUTPUT) ]; then $(RM) $(OUTPUT); fi
