
CXX = g++
CC = $(CXX)
CCFLAGS = -c -std=c++11 -g
CXXFLAGS = $(CCFLAGS)
LDFLAGS = 

ETC_DIR = etc
SRC_DIR = src
TEST_DIR = test
EXP_DIR = exp

DSRCS = $(SRC_DIR)/deque.cc# $(SRC_DIR)/deque_circ.cc
DLIBS = $(DSRCS:.cc=.h)
DOBJS = $(DSRCS:.cc=.o)

ESRCS = $(ETC_DIR)/libatm_check.cc# $(ETC_DIR)/segv_check.cc $(ETC_DIR)/size_check.cc
ELIBS = $(ESRCS:.cc=.h)
EOBJS = $(ESRCS:.cc=.o)
EEXEC = $(ESRCS:%.cc=%)

TSRCS = $(TEST_DIR)/serial.cc $(TEST_DIR)/concurrent.cc $(TEST_DIR)/stress.cc
TLIBS = $(TSRCS:.cc=.h)
TOBJS = $(TSRCS:.cc=.o)
TEXEC = $(TSRCS:%.cc=%)

XSRCS = $(EXP_DIR)/nonblock.cc $(EXP_DIR)/mutex.cc
XLIBS = $(XSRCS:.cc=.h)
XOBJS = $(XSRCS:.cc=.o)
XEXEC = $(XSRCS:%.cc=%)

SRCS = $(DSRCS) $(ESRCS) $(TSRCS) $(XSRCS)
OBJS = $(DOBJS) $(EOBJS) $(TOBJS) $(XOBJS)
EXECS = $(EEXEC) $(TEXEC) $(XEXEC)

all: $(EEXEC) $(TEXEC) $(XEXEC)

$(TEST_DIR)/serial: $(DOBJS) $(TEST_DIR)/serial.o 
	$(CC) $(LDFLAGS) -o $@ $^

$(TEST_DIR)/concurrent: $(DOBJS) $(TEST_DIR)/concurrent.o 
	$(CC) $(LDFLAGS) -o $@ $^ -D_XOPEN_SOURCE=600 -pthread

$(TEST_DIR)/stress: $(DOBJS) $(TEST_DIR)/stress.o 
	$(CC) $(LDFLAGS) -o $@ $^ -D_XOPEN_SOURCE=600 -pthread

$(EXP_DIR)/mutex: $(DOBJS) $(EXP_DIR)/mutex.o $(EXP_DIR)/hrtimer/hrtimer_x86.o
	$(CC) $(LDFLAGS) -o $@ $^ -D_XOPEN_SOURCE=600 -pthread

$(EXP_DIR)/nonblock: $(DOBJS) $(EXP_DIR)/nonblock.o $(EXP_DIR)/hrtimer/hrtimer_x86.o
	$(CC) $(LDFLAGS) -o $@ $^ -D_XOPEN_SOURCE=600 -pthread

$(EXP_DIR)/hrtimer/hrtimer_x86.o: $(EXP_DIR)/hrtimer/hrtimer_x86.cc $(EXP_DIR)/hrtimer/hrtimer_x86.h
	$(CC) $(CCFLAGS) $< -o $@ 

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cc $(SRC_DIR)/%.h
	$(CC) $(CCFLAGS) $(SRC_DIR)/$*.cc -o $@

clean:
	-rm $(EXECS) $(OBJS) 
