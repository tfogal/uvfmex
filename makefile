MEX=mex
OBJ=uvf.mexa64
STDFLAGS=-pthread -shared -fPIC -fno-strict-aliasing -Wall -Wextra
CXXFLAGS=$(STDFLAGS) -DQT_SHARED -DQT_NO_DATESTRING
LDFLAGS=$(STDFLAGS) -pthread -shared -fPIC

all: $(OBJ)

%.mexa64: %.cpp
	$(MEX) CXXFLAGS="\$$CXXFLAGS $(CXXFLAGS)" LDFLAGS="\$$LDFLAGS $(LDFLAGS)" CXXLIBS="\$$CXXLIBS tuvok/Build/libTuvok.a tuvok/IO/expressions/libtuvokexpr.a" -v -g -cxx $^ -o $@ -glnxa64 -g -largeArrayDims -cxx -L/usr/lib64 -lQtOpenGL -lQtGui -lQtCore -lGLU -lGL -lz -Ituvok/IO/3rdParty/boost -Ituvok/Basics -Ituvok

clean:
	rm -f $(OBJ)
