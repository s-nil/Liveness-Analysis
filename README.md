# Liveness-Analysis

To create .ll file of a c/c++ file(hello.c/hello.cpp) 
    clang/clang++ -S -emit-llvm hello.c/hello.cpp

To create .bc file from a c/c++ file
    clang/clang++ -c -emir-llvm hello.c/hello.cpp

To create .bc file from a .ll file
    llvm-as hello.ll

To Compile
    make

To Run
    opt -load ./pass.so -LA < ./tests/hello.ll > /dev/null
    [OR]
    opt -load ./pass.so -LA < ./tests/hello.bc > /dev/null
