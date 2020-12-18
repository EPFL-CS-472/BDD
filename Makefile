CC := g++
CFLAGS := -g -std=c++17
exe = bdd
exe2 = bdd_simple
path = src

all:$(path)/main.cpp $(path)/BDD.hpp $(path)/truth_table.hpp
	@$(CC) $(path)/main.cpp -o $(exe) $(CFLAGS)

simple:$(path)/simple.cpp $(path)/BDD.hpp $(path)/truth_table.hpp
	@$(CC) $(path)/simple.cpp -o $(exe2) $(CFLAGS)

clean:
	@rm -rf *.o *.dSYM $(exe) $(exe2)

