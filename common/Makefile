
.PHONY: all clean
EXE = aladdin
DEBUGGER = debugger/debugger

HAS_READLINE ?= 0

MACHINE_MODEL_OBJS = BaseDatapath.o ScratchpadDatapath.o Scratchpad.o \
                     Registers.o Partition.o LogicalArray.o ReadyPartition.o \
                     SourceManager.o Program.o AladdinExceptions.o LoopInfo.o

GRAPH_OPTS_OBJS = graph_opts/base_opt.o \
									graph_opts/memory_ambiguation.o \
									graph_opts/phi_node_removal.o \
									graph_opts/induction_dependence_removal.o \
									graph_opts/base_address_init.o \
									graph_opts/loop_unrolling.o \
									graph_opts/load_buffering.o \
									graph_opts/store_buffering.o \
									graph_opts/repeated_store_removal.o \
									graph_opts/tree_height_reduction.o \
									graph_opts/reg_load_store_fusion.o \
									graph_opts/consecutive_branch_fusion.o \
									graph_opts/per_loop_pipelining.o \
									graph_opts/global_loop_pipelining.o \
									graph_opts/dma_base_address_init.o

UTILS_OBJS = file_func.o power_func.o opcode_func.o
DEBUGGER_OBJS = debugger/debugger_print.o \
		debugger/debugger_commands.o \
		debugger/debugger_prompt.o
DDDG_OBJS = DDDG.o
OBJS += $(MACHINE_MODEL_OBJS) $(UTILS_OBJS) $(DDDG_OBJS) $(GRAPH_OPTS_OBJS)
OBJ_FILES = $(patsubst %.o,obj/%.o,$(OBJS))
DEBUGGER_OBJ_FILES = $(patsubst %.o,obj/%.o,$(DEBUGGER_OBJS))
WARNING_EXCEPTIONS = -Wno-reorder -Wno-deprecated-declarations

OBJ_DIR = obj

CACTI_DIR = cacti-p
CACTI_OBJ_DIR = $(CACTI_DIR)/obj_opt

ifndef NTHREADS
  NTHREADS = 8
endif
CACTI_OBJS = Ucache.o bank.o cacti_interface.o decoder.o  parameter.o \
             technology.o arbiter.o  basic_circuit.o  component.o  htree2.o \
						 mat.o router.o uca.o area.o crossbar.o io.o nuca.o subarray.o \
						 wire.o powergating.o
CACTI_SRCS = $(patsubst %.o,%.cc,$(CACTI_OBJS))
CACTI_OBJ_FILES = $(patsubst %.o,$(CACTI_OBJ_DIR)/%.o,$(CACTI_OBJS))

CPPFLAGS = -I$(BOOST_ROOT) -I$(BOOST_ROOT)/include -I$(ALADDIN_HOME) -Icacti-p
CFLAGS += -g $(BITWIDTH) -std=c++11 $(CPPFLAGS) -Wno-deprecated-declarations
LFLAGS += -L$(BOOST_ROOT)/lib -lz -lboost_graph -lpthread

ifeq ($(HAS_READLINE),1)
  CFLAGS += -DHAS_READLINE
  LFLAGS += -lreadline
endif

# Get the LLVM version
LLVM_VERSION = $(shell clang --version | grep -o -E "[[:digit:]].[[:digit:]]" | head -n 1 | sed 's/\.//g')
ifneq ($(filter $(LLVM_VERSION), 34 60),)
  CFLAGS += -DLLVM_VERSION=$(LLVM_VERSION)
else
  $(warning We only support LLVM-3.4 and LLVM-6.0!)
  CFLAGS += -DLLVM_VERSION=34
endif

DIRS = $(OBJ_DIR) $(OBJ_DIR)/graph_opts $(OBJ_DIR)/debugger $(CACTI_OBJ_DIR)

all: CFLAGS+=-O3
all: $(EXE)

debug: CFLAGS+=-DDEBUG -O0
debug: $(EXE)

debugger: $(DEBUGGER)

$(DEBUGGER): $(DIRS) $(CACTI_OBJ_FILES) $(OBJ_FILES) $(DEBUGGER_OBJ_FILES)
	$(CXX) $(CFLAGS) -o $@.o -c $@.cpp
	$(CXX)  $(BITWIDTH) -o $@ $@.o $(OBJ_FILES) $(DEBUGGER_OBJ_FILES) $(CACTI_OBJ_FILES) $(LFLAGS)

$(DIRS):
	mkdir -p $(OBJ_DIR)
	mkdir -p $(OBJ_DIR)/graph_opts
	mkdir -p $(OBJ_DIR)/debugger
	mkdir -p $(CACTI_OBJ_DIR)

lib : $(DIRS) $(OBJ_FILES) $(CACTI_OBJ_FILES)
	ar rcs $(ALADDIN_HOME)/libaladdin.a $(OBJ_FILES) $(CACTI_OBJ_FILES)

$(EXE): $(DIRS) $(CACTI_OBJ_FILES) $(OBJ_FILES)
	$(CXX) $(CFLAGS) -c aladdin.cpp
	$(CXX)  $(BITWIDTH) -o $(EXE) $(EXE).o $(OBJ_FILES) $(CACTI_OBJ_FILES) $(LFLAGS)

$(OBJ_DIR)/%.o : %.h %.cpp
	$(CXX) $(CFLAGS) -Werror -Wall $(WARNING_EXCEPTIONS) -o $@ -c $*.cpp

$(CACTI_OBJ_DIR)/%.o : $(CACTI_DIR)/%.cc
	$(CXX) $(CFLAGS)  -DNTHREADS=$(NTHREADS) -o $@ -c $(CACTI_DIR)/$*.cc

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(CACTI_OBJ_DIR)/*.o
	rm -f aladdin
