.PHONY: all clean clean-test report_dir test junit_test

TEST_REPORT_DIR = report_dir

COMMON_OBJS = catch_common.o
TEST_OBJS = test_dddg_generation.o test_init_base_address.o \
            test_dynamic_method_name.o test_loop_pipelining.o \
            test_loop_unrolling.o test_rm_induction_var.o test_rm_phi.o \
            test_spm_part.o test_store_buffer.o \
            test_tree_height_reduction.o test_loop_flatten.o
TESTS = $(patsubst %.o,%,$(TEST_OBJS))

TEST_DEPS = $(ALADDIN_HOME)/libaladdin.a $(COMMON_OBJS)

CFLAGS += -g $(BITWIDTH) -O2 -std=c++0x -I$(BOOST_ROOT) \
         -I$(ALADDIN_HOME)/common
LFLAGS += -L$(BOOST_ROOT)/stage/lib -lboost_graph -lboost_regex \
         -L$(ALADDIN_HOME) -laladdin -lpthread -lz

report_dir:
	@mkdir -p $(TEST_REPORT_DIR)

all : report_dir $(TESTS)

test : report_dir $(TESTS)
	$(foreach t,$(TESTS),./$t;)

junit_test: report_dir $(TESTS)
	$(foreach t,$(TESTS),./$t -r junit -o $(TEST_REPORT_DIR)/$t.xml;)

$(ALADDIN_HOME)/libaladdin.a :
	$(MAKE) -C ../common lib

$(TESTS) : % : %.o $(TEST_DEPS)
	$(CXX) -o $@ $*.o $(COMMON_OBJS) $(LFLAGS)

%.o : %.cpp
	$(CXX) -c $(CFLAGS) $< -o $*.o

clean:
	rm -f *.o
	rm -f $(TESTS) $(COMMON_OBJS)
	rm -f $(ALADDIN_HOME)/libaladdin.a
	rm -rf $(TEST_REPORT_DIR)

clean-test:
	rm -f *.o
	rm -f $(TESTS) $(COMMON_OBJS)
	rm -rf $(TEST_REPORT_DIR)
