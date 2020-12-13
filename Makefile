CC := g++
CFLAGS := -g -std=c++11
exe = bdd
exe2 = bdd_simple
exe3 = bdd_my_tests
path = src

all:$(path)/main.cpp $(path)/BDD.hpp $(path)/truth_table.hpp
	@$(CC) $(path)/main.cpp -o $(exe) $(CFLAGS)

simple:$(path)/simple.cpp $(path)/BDD.hpp $(path)/truth_table.hpp
	@$(CC) $(path)/simple.cpp -o $(exe2) $(CFLAGS)

my_tests:$(path)/my_tests.cpp $(path)/BDD.hpp $(path)/truth_table.hpp
	@$(CC) $(path)/my_tests.cpp -o $(exe3) $(CFLAGS)

clean:
	@rm -rf *.o *.dSYM $(exe) $(exe2) $(exe3)

