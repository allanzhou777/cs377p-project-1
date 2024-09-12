# Define the compiler and flags
CC = gcc
CFLAGS = -lpapi -O3 

# Define the target executable
TARGET = mmult

# Default target to compile the project
all: $(TARGET)

# Rule to build the mmult executable
$(TARGET): mmult.c
	$(CC) -o $(TARGET) mmult.c $(CFLAGS)

# Clean target to remove the compiled executable
clean:
	rm -f $(TARGET)
