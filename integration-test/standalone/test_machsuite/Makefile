# This test runs the aes benchmark through the gem5 infrastructure and performs
# the output validation to show that the infrastructure is modeling all loads
# and stores correctly.

.PHONY: test run

trace:
	$(MAKE) -C $(ALADDIN_HOME)/MachSuite/aes/aes autotrace
	cp $(ALADDIN_HOME)/MachSuite/aes/aes/dynamic_trace.gz .

run: test
	@echo off
	rm -f output.data
	sh run.sh

test:
	$(MAKE) -C $(ALADDIN_HOME)/MachSuite/aes/aes gem5
