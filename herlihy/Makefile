
CXX = g++
CC = $(CXX)
CCFLAGS = -c -std=c++11
CXXFLAGS = $(CCFLAGS)
LDFLAGS = 

ETC_DIR = etc
SRC_DIR = src
TEST_DIR = test

DSRCS = $(SRC_DIR)/deque.cc # $(SRC_DIR)/deque_circ.cc
DLIBS = $(DSRCS:.cc=.h)
DOBJS = $(DSRCS:.cc=.o)

ESRCS = $(ETC_DIR)/libatm_check.cc $(ETC_DIR)/segv_check.cc $(ETC_DIR)/size_check.cc
ELIBS = $(ESRCS:.cc=.h)
EOBJS = $(ESRCS:.cc=.o)
EEXEC = $(ESRCS:%.cc=%)

TSRCS = 
TLIBS = $(TSRCS:.cc=.h)
TOBJS = $(TSRCS:.cc=.o)
TEXEC = $(TSRCS:%.cc=%)

SRCS = $(DSRCS) $(ESRCS) $(TSRCS)
OBJS = $(DOBJS) $(EOBJS) $(TOBJS)
EXECS = $(EEXEC) $(TEXEC)

all: $(TEXEC)

depend:
	makedepend -Y $(SRCS)

clean:
	-rm $(EXECS) $(OBJS) 
