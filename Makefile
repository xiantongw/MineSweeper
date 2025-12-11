.PHONY: clean

MineSweeper: main.cpp
	g++ -std=c++17 -o MineSweeper main.cpp

clean:
	rm -rf MineSweeper
