TARGET=task

all:
	clang++ -g ${TARGET}.cc -o ${TARGET} -std=c++17 -Wall