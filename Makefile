.PHONY: clean

main: main.cpp
	clang++ -std=c++17 -Wall -Wextra -O0 -g -fsanitize=address,undefined -o main main.cpp

clean:
	rm -rf main main.dSYM