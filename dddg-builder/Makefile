OBJS = dddg.o 

libdddgbuild : $(OBJS)
	$(CXX) -shared -fPIC -o ../lib/libdddgbuild.so $(OBJS)
	rm *.o

dddg.o : dddg.h dddg.cpp
	$(CXX) $(CFLAGS) dddg.cpp
  
clean:
	rm *.o



