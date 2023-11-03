# Compiler
CC = gcc
CFLAGS = -Wall -pthread

# Source files and output executable
SOURCES = main.c list.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = s_talk

# Default target
all: $(EXECUTABLE)

# Rule to build the executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

# Rule to compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: all clean
