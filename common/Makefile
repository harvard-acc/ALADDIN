####### USER OPTIONS ########
# Uncomment the following line to build with MySQL dependencies.
# USE_DB = 1
#############################

.PHONY: all clean
EXE = aladdin

MACHINE_MODEL_OBJS = BaseDatapath.o ScratchpadDatapath.o Scratchpad.o \
                     Registers.o
UTILS_OBJS = file_func.o generic_func.o power_func.o
DDDG_OBJS = DDDG.o
OBJS += $(MACHINE_MODEL_OBJS) $(UTILS_OBJS) $(DDDG_OBJS)
OBJ_FILES = $(patsubst %.o,obj/%.o,$(OBJS))

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

DEBUG =
CFLAGS += $(DEBUG) $(BITWIDTH) -O2 -std=c++0x -I$(BOOST_ROOT) -Icacti-p
LFLAGS += -lz -L$(BOOST_ROOT)/stage/lib -lboost_graph -lboost_regex -lpthread

ifeq ($(USE_DB), 1)
ifeq ($(wildcard $(MYSQL_HOME)),)
    $(error Set MYSQL_HOME to a valid location)
endif
LFLAGS += -lmysqlcppconn
CFLAGS += -I$(MYSQL_HOME)/include -DUSE_DB
OBJS += DatabaseConfig.o
endif

all : $(EXE)

obj_dir:
	mkdir -p $(OBJ_DIR)
	mkdir -p $(CACTI_OBJ_DIR)

lib : obj_dir $(OBJ_FILES) $(CACTI_OBJ_FILES)
	ar rcs $(ALADDIN_HOME)/libaladdin.a $(OBJ_FILES) $(CACTI_OBJ_FILES)

$(EXE): obj_dir $(CACTI_OBJ_FILES) $(OBJ_FILES)
	$(CXX) $(CFLAGS) -c aladdin.cpp
	$(CXX)  $(BITWIDTH) -o $(EXE) $(EXE).o $(OBJ_FILES) $(CACTI_OBJ_FILES) $(LFLAGS)

$(OBJ_DIR)/%.o : %.h %.cpp
	$(CXX) $(CFLAGS) -o $@ -c $*.cpp

$(CACTI_OBJ_DIR)/%.o : $(CACTI_DIR)/%.cc
	$(CXX) $(CFLAGS)  -DNTHREADS=$(NTHREADS) -o $@ -c $(CACTI_DIR)/$*.cc

clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(CACTI_OBJ_DIR)/*.o
	rm -f aladdin
