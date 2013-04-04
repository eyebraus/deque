
BIN = deque
CC = gcc
CCFLAGS = -c
LDFLAGS = 
CSRCS = $(BIN).c
HSRCS = $(CSRCS:.c=.h)
OBJS = $(CSRCS:.c=.o)

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(BIN)

$(OBJS): $(CSRCS) $(HSRCS)
	$(CC) $(CCFLAGS) -c $(CSRCS)

clean:
	-rm $(OBJS) $(BIN) $(DEPS)
