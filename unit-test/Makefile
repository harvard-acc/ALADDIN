.PHONY: all clean
TEST_OBJS = test_dddg_generation.o test_init_base_address.o \
            test_loop_pipelining.o \
            test_loop_unrolling.o test_rm_induction_var.o test_rm_phi.o \
            test_spm_part.o test_store_buffer.o \
            test_tree_height_reduction.o test_loop_flatten.o
TESTS = $(patsubst %.o,%,$(TEST_OBJS))

MACHINE_MODEL_OBJS = $(ALADDIN_HOME)/BaseDatapath.o \
                     $(ALADDIN_HOME)/ScratchpadDatapath.o \
                     $(ALADDIN_HOME)/Scratchpad.o
UTILS_OBJS = $(ALADDIN_HOME)/file_func.o $(ALADDIN_HOME)/generic_func.o \
             $(ALADDIN_HOME)/opcode_func.o
DDDG_OBJS = $(ALADDIN_HOME)/DDDG.o
TEST_DEPS = $(MACHINE_MODEL_OBJS) $(UTILS_OBJS) $(DDDG_OBJS)

CFLAGS = $(DEBUG) $(BITWIDTH) -O2 -std=c++0x -I$(BOOST_ROOT) -I$(ALADDIN_HOME)
LFLAGS = $(DEBUG) -lz -L$(BOOST_ROOT)/stage/lib  -lboost_graph -lboost_regex

all : $(TESTS)

$(TESTS) : % : %.o $(TEST_DEPS)
	$(CXX) $(CFLAGS) -o $@ $(TEST_DEPS) $*.o $(LFLAGS)

%.o : %.cpp
	$(CXX) -c $(CFLAGS) $< -o $*.o

clean:
	rm -f *.o
	rm -f $(TESTS)
