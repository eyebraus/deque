
BIN = deque
SBIN = deque_circ
OBIN = size_check segv_check
CC = g++
CCFLAGS = -c
LDFLAGS = 
CSRCS = $(BIN).c $(SBIN).c
HSRCS = $(CSRCS:.c=.h)
OBJS = $(CSRCS:.c=.o)

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(BIN)

$(SBIN): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(SBIN)

$(OBJS): $(CSRCS) $(HSRCS)
	$(CC) $(CCFLAGS) $(CSRCS)

clean:
	-rm $(OBJS) $(BIN) $(SBIN) $(OBIN) $(DEPS)
