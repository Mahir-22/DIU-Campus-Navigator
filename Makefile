CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -pedantic
LDFLAGS = -lm
TARGET = campus_navigator
SRC = src/campus_navigator.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe *.o *.obj

.PHONY: all run clean
