CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99

TARGET = user
SRCS = signals.c user.c args.c validation.c ds_protocol.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
