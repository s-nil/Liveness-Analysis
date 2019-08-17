LLVM_CONFIG=llvm-config

CXX=clang++
CXXFLAGS=`$(LLVM_CONFIG) --cppflags` -fPIC -fno-rtti
LDFLAGS=`$(LLVM_CONFIG) --ldflags`

all: pass.so

pass.so: pass.o
	$(CXX) -shared pass.o -o pass.so $(LDFLAGS)

pass.o: pass.cpp
	$(CXX) -c pass.cpp -o pass.o $(CXXFLAGS)

clean:
	rm -f *.o *.so
