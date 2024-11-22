# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -g
LDFLAGS =

# Directories
SRC_DIRS = . balancer cache server structures utils
SRCS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
OBJS = $(SRCS:.c=.o)

# Target Executable
TARGET = program

# Default Rule
all: $(TARGET)

# Build the Target
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile Source Files to Object Files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean Up Build Artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Phony Targets
.PHONY: all clean