
ifndef CXX
	CXX=g++
endif

LIBS += -lpthread -lopenblas
CPPFLAGS += -g

canonical_path := ../$(shell basename $(shell pwd -P))

all: tool

%.o: %.cpp $(wildcard *.hpp)
	$(CXX) $(CPPFLAGS) -c -o $@ ${canonical_path}/$<

tool: test.o runtime.o kblas.o
	$(CXX) -o $@ -std=c++11 $^ $(LIBS) $(LDFLAGS)

clean:
	rm -rf *.o tool
