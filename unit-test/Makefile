.PHONY: all clean
TESTS = test_graph_trans
TEST_OBJS = test_graph_trans.o

MACHINE_MODEL_OBJS = $(ALADDIN_HOME)/BaseDatapath.o $(ALADDIN_HOME)/ScratchpadDatapath.o $(ALADDIN_HOME)/Scratchpad.o
UTILS_OBJS = $(ALADDIN_HOME)/file_func.o $(ALADDIN_HOME)/generic_func.o $(ALADDIN_HOME)/opcode_func.o
DDDG_OBJS = $(ALADDIN_HOME)/DDDG.o
TEST_DEPS = $(MACHINE_MODEL_OBJS) $(UTILS_OBJS) $(DDDG_OBJS) $(TEST_OBJS)

CFLAGS = $(DEBUG) $(BITWIDTH) -O2 -std=c++0x -I$(BOOST_ROOT) -I$(ALADDIN_HOME)
LFLAGS = $(DEBUG) -lz -L$(BOOST_ROOT)/stage/lib  -lboost_graph -lboost_regex

all : $(TESTS)

$(TESTS) : $(TEST_DEPS)
	$(CXX) $(CFLAGS) -o $@ $(TEST_DEPS) $(LFLAGS)

%.o : %.cpp
	$(CXX) -c $(CFLAGS) $< -o $*.o

clean:
	rm -f *.o
	rm -f $(TESTS)
