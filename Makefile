CC = g++
LDFLAGS =
CXXFLAGS = -Wall
CPPFLAGS = 
INCLUDES = -I./src/third_party/tclap-1.2.2/include -I./src/common/include
LIBS = 
SOURCEDIR := .
SOURCE := src/lz77/main.cpp src/common/src/LZ77.cpp src/common/src/BitBarrel.cpp src/common/src/BitBarrelWriter.cpp
OBJ = $(SOURCE:.cpp=.o)
EXEC = lz77

DEBUG=no
ifeq ($(DEBUG),yes)
    CXXFLAGS += -g
endif

.PHONY: depend clean

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $(EXEC) $(OBJ) $(LDFLAGS) $(LIBS)
	
.cpp.o:
	$(CC) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f *.o *~ $(OBJ) $(EXEC)

depend: $(SOURCE)
	makedepend $(INCLUDES) $^
