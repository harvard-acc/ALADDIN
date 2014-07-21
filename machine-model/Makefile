OBJS = Datapath.o Scratchpad.o
libmachinemodel : $(OBJS)
	$(CXX) -shared -fPIC -o ../lib/libmachinemodel.so $(OBJS)
	rm *.o

Datapath.o : Datapath.h Datapath.cpp
	$(CXX) $(CFLAGS) Datapath.cpp

Scratchpad.o : Scratchpad.h Scratchpad.cpp
	$(CXX) $(CFLAGS) Scratchpad.cpp

clean:
	rm *.o



