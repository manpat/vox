GCC = g++-5
SFLAGS = -Iinclude -I./ -I/usr/local/include/ -I/usr/local/include/bullet/ 
SFLAGS+= -std=c++14 -Wall -Wextra -Wpedantic -O1 -g
LFLAGS = `pkg-config --libs bullet` -lSDL2 -lSDL2_ttf -lSDL2_image -lGL -O1 -g
SRC=$(shell find src -name "*.cpp")
OBJ=$(SRC:%.cpp=%.o)

parallelbuild:
	@make build -j8 --silent

build: $(OBJ) 
	@echo "-- Linking --"
	@$(GCC) $(OBJ) $(LFLAGS) -obuild

src/main.cpp: include/app.h
	@touch src/main.cpp

src/block.cpp: blocks/*.h
	@touch src/block.cpp

%.o: %.cpp %.h
	@echo "-- Generating $@ --"
	@$(GCC) $(SFLAGS) -c $< -o $@

%.o: %.cpp
	@echo "-- Generating $@ --"
	@$(GCC) $(SFLAGS) -c $< -o $@

run: parallelbuild
	@echo "-- Running --"
	@./build

clean:
	@echo "-- Cleaning --"
	@rm -f *.o
	