LLVM_HOME?=PLEASE_SET_LLVM_HOME_ENVIRONMENT_VARIABLE
LLVM_SRC=$(LLVM_HOME)/llvm-src
LLVM_BIN=$(LLVM_HOME)/
#LLVM_BIN=$(LLVM_HOME)/llvm-bin

all: full_trace.so

SYSTEM=$(shell uname)
ifeq ($(SYSTEM), Darwin)
	CFLAGS=  -fno-common 
	LDFLAGS=  -dynamiclib -Wl,-flat_namespace -Wl,-undefined,suppress 
endif
ifeq ($(SYSTEM), Linux)
	CFLAGS= -O3 -fPIC 
	LDFLAGS=  -shared 
endif

full_trace.so  : %.so: %.cpp
	$(LLVM_BIN)/bin/clang++ -v $(CFLAGS) -c $< -o tmp.o `$(LLVM_BIN)/bin/llvm-config --cxxflags`
	$(LLVM_BIN)/bin/clang++ -v $(LDFLAGS) -o $@ tmp.o
	rm -f tmp.o

clean:
	rm -f full_trace.so
