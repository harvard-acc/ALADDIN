# This test runs the aes benchmark through the gem5 infrastructure and performs
# the output validation to show that the infrastructure is modeling all loads
# and stores correctly.

.PHONY: all trace run

all: aes trace

aes:
	$(MAKE) -C $(ALADDIN_HOME)/MachSuite/aes/aes gem5-accel
	cp $(ALADDIN_HOME)/MachSuite/aes/aes/aes-aes-gem5-accel aes

trace:
	$(MAKE) -C $(ALADDIN_HOME)/MachSuite/aes/aes clean-trace
	$(MAKE) -C $(ALADDIN_HOME)/MachSuite/aes/aes trace-binary
	$(MAKE) -C $(ALADDIN_HOME)/MachSuite/aes/aes run-trace
	cp $(ALADDIN_HOME)/MachSuite/aes/aes/dynamic_trace.gz .

run: aes
	@echo off
	rm -f output.data
	sh run.sh


clean:
	rm -rf aes
	rm -rf dynamic_trace.gz
	rm -rf output.data
