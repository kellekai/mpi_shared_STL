#BOOST_ROOT=/fefs/scratch/bsc93/bsc93655/opt/boost/1.77.0
BOOST_INC=$(BOOST_ROOT)/include
BOOST_LIB=$(BOOST_ROOT)/lib

CXX=mpicxx
CXXFLAGS=-I$(BOOST_INC) -g
LDFLAGS=-L$(BOOST_LIB) -lboost_serialization

SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)

all: main2

%.o: %.cpp
		$(CXX) -c $< $(CXXFLAGS)

main2: $(OBJ)
		$(CXX) -o $@ $^ $(LDFLAGS)

clean:
		rm -r *.o

.PHONY: clean

