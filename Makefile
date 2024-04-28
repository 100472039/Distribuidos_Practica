CC = gcc
CFLAGS = -Wall -Wextra -pedantic -pthread -std=c99

SRCS = servidor.c send-recv.c
OBJS = $(SRCS:.c=.o)
TARGET = servidor

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $