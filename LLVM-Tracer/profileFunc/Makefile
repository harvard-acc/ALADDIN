LLVM_HOME?=PLEASE_SET_LLVM_HOME_ENVIRONMENT_VARIABLE
LLVM_SRC=$(LLVM_HOME)/llvm-src
LLVM_BIN=$(LLVM_HOME)/

#CFLAGS=-O3
CFLAGS= -O3 -fPIC 

trace_logger.llvm: trace_logger.c
	$(LLVM_BIN)/bin/clang $(CFLAGS) -S -emit-llvm -c trace_logger.c -o trace_logger.llvm

clean:
	rm -f trace_logger.llvm
