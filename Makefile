CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99

# Geração automática das dependências dos .h (ficheiros .d), para que
# alterar um header recompile os .c que o incluem.
DEPFLAGS = -MMD -MP

TARGET = user
SRCS = signals.c user.c args.c validation.c parse.c ds_protocol.c ds_tcp.c
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)

.PHONY: all clean