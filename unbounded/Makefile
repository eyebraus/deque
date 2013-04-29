
CXX = g++
CC = $(CXX)
CCFLAGS = -c -std=c++11 -g -O0
CXXFLAGS = $(CCFLAGS)
LDFLAGS = 

ETC_DIR = etc
SRC_DIR = src
TEST_DIR = test

DSRCS = $(SRC_DIR)/deque.cc# $(SRC_DIR)/deque_circ.cc
DLIBS = $(DSRCS:.cc=.h)
DOBJS = $(DSRCS:.cc=.o)

ESRCS = $(ETC_DIR)/size_check.cc
ELIBS = $(ESRCS:.cc=.h)
EOBJS = $(ESRCS:.cc=.o)
EEXEC = $(ESRCS:%.cc=%)

TSRCS = $(TEST_DIR)/serial.cc #$(TEST_DIR)/concurrent.cc $(TEST_DIR)/stress.cc
TLIBS = $(TSRCS:.cc=.h)
TOBJS = $(TSRCS:.cc=.o)
TEXEC = $(TSRCS:%.cc=%)

SRCS = $(DSRCS) $(ESRCS) $(TSRCS)
OBJS = $(DOBJS) $(TOBJS)
EXECS = $(EEXEC) $(TEXEC)

#all: $(EEXEC) $(TEXEC)

#$(DOBJS): $(@:.o=.c)
	#@echo making $@
	#$(CC) $(CCFLAGS) $(@:.o=.c) -o $@

#$(TEXEC): $@.o $(DOBJS)
	#@echo making $@
	#$(CC) $(LDFLAGS) -o $@ $^

#$(TOBJS): $(@:.o=.c)
	#@echo making $@
	#$(CC) $(CCFLAGS) $(@:.o=.c) -o $@

all: $(EEXEC) $(TEXEC)

$(ETC_DIR)/size_check: $(ETC_DIR)/size_check.o
	$(CC) $(LDFLAGS) -o $@ $^

$(TEST_DIR)/serial: $(DOBJS) $(TEST_DIR)/serial.o 
	$(CC) $(LDFLAGS) -o $@ $^

$(TEST_DIR)/concurrent: $(DOBJS) $(TEST_DIR)/concurrent.o 
	$(CC) $(LDFLAGS) -o $@ $^ -D_XOPEN_SOURCE=600 -pthread

$(TEST_DIR)/stress: $(DOBJS) $(TEST_DIR)/stress.o 
	$(CC) $(LDFLAGS) -o $@ $^ -D_XOPEN_SOURCE=600 -pthread

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cc $(SRC_DIR)/%.h
	$(CC) $(CCFLAGS) $(SRC_DIR)/$*.cc -o $@

clean:
	-rm $(EXECS) $(OBJS) 
