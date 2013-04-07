
BIN = deque
OBIN = size_check segv_check
CC = g++
CCFLAGS = -c
LDFLAGS = 
CSRCS = $(BIN).c
HSRCS = $(CSRCS:.c=.h)
OBJS = $(CSRCS:.c=.o)

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(BIN)

$(OBJS): $(CSRCS) $(HSRCS)
	$(CC) $(CCFLAGS) $(CSRCS)

clean:
	-rm $(OBJS) $(BIN) $(OBIN) $(DEPS)
