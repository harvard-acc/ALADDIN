.PHONY: all clean
EXE = aladdin

MACHINE_MODEL_OBJS = BaseDatapath.o ScratchpadDatapath.o Scratchpad.o
UTILS_OBJS = file_func.o generic_func.o opcode_func.o
DDDG_OBJS = DDDG.o
OBJS = $(MACHINE_MODEL_OBJS) $(UTILS_OBJS) $(DDDG_OBJS)
CFLAGS = -c $(DEBUG) $(BITWIDTH) -O2 -std=c++0x -I$(BOOST_ROOT)
LFLAGS = $(DEBUG) -lz -L$(BOOST_ROOT)/stage/lib  -lboost_graph -lboost_regex

all : $(EXE)

lib : $(OBJS)
	ar rcs libaladdin.a $(OBJS)

$(EXE): $(OBJS)
	$(CXX) $(CFLAGS) aladdin.cpp
	$(CXX)  $(BITWIDTH) -o $(EXE) $(EXE).o $(OBJS) $(LFLAGS)

%.o : %.h %.cpp
	$(CXX) $(CFLAGS) -o $@ $*.cpp

clean:
	rm -f *.o
	rm -f */*.o
	rm -f aladdin
