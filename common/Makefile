
.PHONY: all clean
EXE = aladdin

HAS_READLINE ?= 0

MACHINE_MODEL_OBJS = BaseDatapath.o ScratchpadDatapath.o Scratchpad.o \
                     Registers.o Partition.o LogicalArray.o ReadyPartition.o \
                     SourceManager.o Program.o AladdinExceptions.o

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

UTILS_OBJS = file_func.o generic_func.o power_func.o
DEBUGGER_OBJS = debugger_print.o debugger_commands.o debugger_prompt.o
DDDG_OBJS = DDDG.o
OBJS += $(MACHINE_MODEL_OBJS) $(UTILS_OBJS) $(DDDG_OBJS) $(GRAPH_OPTS_OBJS)
OBJ_FILES = $(patsubst %.o,obj/%.o,$(OBJS))
DEBUGGER_OBJ_FILES = $(patsubst %.o,obj/%.o,$(DEBUGGER_OBJS))
WARNING_EXCEPTIONS = -Wno-reorder

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

CFLAGS += -g $(BITWIDTH) -std=c++0x -I$(BOOST_ROOT) -Icacti-p \
					-I$(ALADDIN_HOME)
LFLAGS += -lz -L$(BOOST_ROOT)/stage/lib -lboost_graph -lboost_regex -lpthread

ifeq ($(HAS_READLINE),1)
  CFLAGS += -DHAS_READLINE
  LFLAGS += -lreadline
endif

all: CFLAGS+=-O3
all: $(EXE)

debug: CFLAGS+=-DDEBUG -O0
debug: $(EXE)

debugger: obj_dir $(CACTI_OBJ_FILES) $(OBJ_FILES) $(DEBUGGER_OBJ_FILES)
	$(CXX) $(CFLAGS) -c $@.cpp
	$(CXX)  $(BITWIDTH) -o $@ $@.o $(OBJ_FILES) $(DEBUGGER_OBJ_FILES) $(CACTI_OBJ_FILES) $(LFLAGS)

obj_dir:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(OBJ_DIR)/graph_opts
	mkdir -p $(CACTI_OBJ_DIR)

lib : obj_dir $(OBJ_FILES) $(CACTI_OBJ_FILES)
	ar rcs $(ALADDIN_HOME)/libaladdin.a $(OBJ_FILES) $(CACTI_OBJ_FILES)

$(EXE): obj_dir $(CACTI_OBJ_FILES) $(OBJ_FILES)
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
