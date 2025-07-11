CC = gcc
CFLAGS = -Wall -std=c99 -Iinclude
AR = ar
ARFLAGS = rcs

SRCDIR = src
INCDIR = include
OBJDIR = obj
LIBDIR = lib

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
LIBRARY = $(LIBDIR)/libjblang_runtime.a

.PHONY: all clean

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(LIBDIR):
	mkdir -p $(LIBDIR)

clean:
	rm -rf $(OBJDIR) $(LIBDIR)
