CC = gcc
CFLAGS = -Wall -Wextra -std=c99

SRCS = message.c
OBJS = $(SRCS:.c=.o)
TARGET = message

all: $(TARGET)

$(TARGET): $(OBJS)
        $(CC) $(CFLAGS) -o $@ $^

%.o: %.c
        $(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
        ./$(TARGET)

clean:
        rm -f $(OBJS) $(TARGET)
