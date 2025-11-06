# Define the compiler and flags
CC = gcc

# Define the name of the final executable
TARGET = shell.out

# List all the source files (.c) within the src/ directory
SRCS = src/runner.c src/a3.c src/e1.c src/b1.c src/b3.c src/name.c

# Define the object files (.o) that will be generated from the source files
OBJS = $(patsubst src/%.c,src/%.o,$(SRCS))

# The default target for 'make pp'
all: $(TARGET)

# Rule to link the object files into the final executable
$(TARGET): $(OBJS)
	$(CC) $^ -o $@

# Generic rule to compile each .c file into a .o file
# It now includes the include/ directory for header files
src/%.o: src/%.c
	$(CC) -Iinclude -c $< -o $@

# Rule to run the executable from the current directory
run: $(TARGET)
	./$(TARGET)

# Rule to clean up the generated files
clean:
	rm -f $(OBJS) $(TARGET)
