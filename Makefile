CC = clang
CFLAGS = -g #-Wall -Werror
SRCDIR = src
OBJDIR = obj
BINDIR = bin
TARGET = $(BINDIR)/clox

# Find all .c files recursively
SRCS = $(shell find $(SRCDIR) -type f -name "*.c")
# Generate corresponding .o file names
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
# Generate include directories
INCLUDES = -I$(SRCDIR) $(shell find $(SRCDIR)/lib -type d -exec echo -I{} \;)

.PHONY: all clean run bear

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJDIR) $(BINDIR)

bear:
	bear -- make
