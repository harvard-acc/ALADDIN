# Aladdin unit tests.
#
# To build unit tests:
# 	make all -j8
#
# To run unit tests with junit output:
#    make junit_test [-jN]
# where -jN can optionally parallelize the operation over N threads.
# The output will be written to report_dir/.
#
# To run all unit tests one by one to see the raw output:
# 	make test
# With this target, tests cannot be run in parallel (since each test's output
# will get then interleaved).
#
# To build the performance test (which will just measure how long it takes to
# run Aladdin on a set of traces):
# 	make perf
# 	./test_performance

.PHONY: all clean clean-test report_dir test junit_test

ifeq ($(USE_DB), 1)
ifeq ($(wildcard $(MYSQL_HOME)),)
    $(error Set MYSQL_HOME to a valid location)
endif
LFLAGS += -lmysqlcppconn
CFLAGS += -I$(MYSQL_HOME)/include -DUSE_DB
endif

TEST_REPORT_DIR = report_dir

COMMON_OBJS = catch_common.o
TEST_OBJS = test_dddg_generation.o test_init_base_address.o \
            test_dynamic_method_name.o test_loop_pipelining.o \
            test_loop_unrolling.o test_rm_induction_var.o test_rm_phi.o \
            test_spm_part.o test_store_buffer.o \
            test_tree_height_reduction.o test_loop_flatten.o \
            test_dma.o test_reg_load_store_fusion.o test_memory_ambiguation.o \
            test_special_math_op.o test_loop_sampling

TESTS = $(patsubst %.o,%,$(TEST_OBJS))

TEST_DEPS = $(ALADDIN_HOME)/libaladdin.a $(COMMON_OBJS)

CFLAGS += -g $(BITWIDTH) -std=c++0x -I$(BOOST_ROOT) -I$(BOOST_ROOT)/include \
         -I$(ALADDIN_HOME)/common -I$(ALADDIN_HOME) -Wno-deprecated-declarations
LFLAGS += -L$(BOOST_ROOT)/lib -L$(ALADDIN_HOME) -laladdin -lboost_graph -lpthread -lz

# Get the LLVM version
LLVM_VERSION = $(shell clang --version | grep -o -E "[[:digit:]].[[:digit:]]" | head -n 1 | sed 's/\.//g')
ifneq ($(filter $(LLVM_VERSION), 34 60),)
  CFLAGS += -DLLVM_VERSION=$(LLVM_VERSION)
else
  $(warning We only support LLVM-3.4 and LLVM-6.0!)
  CFLAGS += -DLLVM_VERSION=34
endif

all: CFLAGS+=-DDEBUG -O3
all: report_dir $(TESTS)

debug: CFLAGS+=-DDEBUG -O0
debug: report_dir $(TESTS)

report_dir:
	@mkdir -p $(TEST_REPORT_DIR)

perf: test_performance.o
	$(CXX) -o test_performance $^ $(LFLAGS)

RUN_ALL_TESTS = $(patsubst %, run_test-%, $(TESTS))
run_test-%: report_dir %
	@printf "Running $*..." && \
		./$* > /dev/null && ret=$$? && \
		test $$ret && printf "PASSED\n" || printf "FAILED\n" && \
		test $$ret
test: $(RUN_ALL_TESTS)

# Enables running tests in parallel by making a fake target for each test.
JUNIT_TEST_PARALLEL = $(patsubst %, run_junit-%, $(TESTS))
run_junit-%: report_dir %
	./$* -r junit -o $(TEST_REPORT_DIR)/$*.xml
junit_test: $(JUNIT_TEST_PARALLEL)

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

clean-perf:
	rm test_performance.o
	rm test_performance
