.PHONY: all clean

ifeq ($(USE_DB), 1)
ifeq ($(wildcard $(MYSQL_HOME)),)
    $(error Set MYSQL_HOME to a valid location)
endif
endif

TEST_OBJS = test_dddg_generation.o test_init_base_address.o \
            test_dynamic_method_name.o test_loop_pipelining.o \
            test_loop_unrolling.o test_rm_induction_var.o test_rm_phi.o \
            test_spm_part.o test_store_buffer.o \
            test_tree_height_reduction.o test_loop_flatten.o
TESTS = $(patsubst %.o,%,$(TEST_OBJS))

TEST_DEPS = $(ALADDIN_HOME)/libaladdin.a

CFLAGS = -g $(BITWIDTH) -O2 -std=c++0x -I$(BOOST_ROOT) \
         -I$(ALADDIN_HOME)/common -I$(MYSQL_HOME)
LFLAGS = -g -lz -L$(BOOST_ROOT)/stage/lib  -lboost_graph -lboost_regex \
         -L$(ALADDIN_HOME) -laladdin -lmysqlcppconn

all : $(TESTS)

test : $(TESTS)
	@for unit_test in $(TESTS) ; do \
    echo $$unit_test ; \
		./$$unit_test ; \
	done

$(ALADDIN_HOME)/libaladdin.a :
	$(MAKE) -C ../common lib

$(TESTS) : % : %.o $(TEST_DEPS)
	$(CXX) $(CFLAGS) -o $@ $*.o $(LFLAGS)

%.o : %.cpp
	$(CXX) -c $(CFLAGS) $< -o $*.o

clean:
	rm -f *.o
	rm -f $(TESTS)
	rm -f $(ALADDIN_HOME)/libaladdin.a
