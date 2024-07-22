# Compiler
CC  = gcc

INCLUDES = -I.
SOURCES = hashmap3.o snappy_bytes.o snappy_comDecom.o

RAW ?= 0
DECOMPRESS ?= 0
ifeq ($(DECOMPRESS), 1)
	EXE  = snappyDecompressor
	SOURCES += snappyDecompressor.o
else
	ifeq ($(RAW),1)
		EXE  = snappy_raw
		SOURCES += snappy_raw.o
	else
		EXE  = snappy_framed
		SOURCES += snappy_framed.o
	endif
endif

LIBS = -lm

# Comment/uncomment to enable/disable debugging code
DEBUG ?= 0
ifeq ($(DEBUG),1)
	CFLAGS += -g -O0
else
	CFLAGS += -O3 -Wall 
endif

# Rules
all: sw
sw: $(EXE)

%.o: %.c
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<

$(EXE): $(SOURCES)
	$(CC) $(SOURCES) -o $@ $(LIBS)

.PHONY: clean
clean::
	rm -f $(SOURCES)
	rm -f $(EXE)
