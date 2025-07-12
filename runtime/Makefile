CC = gcc
CFLAGS = -Wall -std=c99 -Iinclude $(ALLOCATOR_FLAGS) $(DEBUG_FLAGS)
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

simple:
	$(MAKE) ALLOCATOR_FLAGS="" DEBUG_FLAGS="$(if $(DEBUG),-DDEBUG,)"

reference_count:
	$(MAKE) ALLOCATOR_FLAGS="-DUSE_REF_COUNT" DEBUG_FLAGS="$(if $(DEBUG),-DDEBUG,)"

mark_sweep:
	$(MAKE) ALLOCATOR_FLAGS="-DUSE_MARK_SWEEP" DEBUG_FLAGS="$(if $(DEBUG),-DDEBUG,)"
