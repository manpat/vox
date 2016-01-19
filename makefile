GCC = g++
SFLAGS = -I./ -I/usr/local/include/ -I/usr/local/include/bullet/ 
SFLAGS+= -std=c++11 -Wall -Wextra -Wpedantic -O1 -g
LFLAGS = `pkg-config --libs bullet` -lSDL2 -lGL -O1 -g
SRC=$(shell find . -name "*.cpp")
OBJ=$(SRC:%.cpp=%.o)

parallelbuild:
	@make build -j8 --silent

build: $(OBJ) 
	@echo "-- Linking --"
	@$(GCC) $(OBJ) $(LFLAGS) -obuild

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
	@rm -f $(OBJ)
	