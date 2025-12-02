SRCDIR = src
VENDORDIR = vendor
OBJDIR = obj
BINARY = span.bin
VENDOR_INCDIR = $(VENDORDIR)/include
VENDOR_LIBDIR = $(VENDORDIR)/lib

COMP = gcc
COMMON_COMPFLAGS = -Wall -Wextra -pedantic -I$(VENDOR_INCDIR)
COMPFLAGS = -ggdb
LDFLAGS = -L$(VENDOR_LIBDIR) -l:libraylib.a -l:libumka.a -lm

# SOURCES = $(wildcard $(SRCDIR)/*.c)
SOURCES = $(SRCDIR)/span.c $(SRCDIR)/ffmpeg_linux.c $(SRCDIR)/main.c
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

.PHONY: all clean compile

all: $(BINARY)

$(BINARY): $(OBJECTS)
	$(COMP) $^ -o $@ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(COMP) $(COMMON_COMPFLAGS) $(COMPFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)/*
