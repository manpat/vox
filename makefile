GCC = g++-5

SharedSFlags = -I. -Iinclude -Iinclude/shared -Iinclude/ext -I/usr/local/include/ -I/usr/local/include/bullet/
SharedSFlags+= -std=c++14 -Wall -Wextra -O1 -g
ClientSFlags:= $(SharedSFlags) -Iinclude/client
ServerSFlags:= $(SharedSFlags) -Iinclude/server

SharedLFlags = `pkg-config --libs bullet` -lRakNetLibStatic -pthread -O1 -g
ClientLFlags:= $(SharedLFlags) -lSDL2 -lSDL2_image -lGL
ServerLFlags:= $(SharedLFlags)

SharedSrc = $(shell find src/shared -name "*.cpp")
ServerSrc = $(shell find src/server -name "*.cpp")
ClientSrc = $(shell find src/client -name "*.cpp")
SharedObj = $(SharedSrc:src/shared/%.cpp=obj/shared/%.o)
ClientObj:= $(ClientSrc:src/client/%.cpp=obj/client/%.o) $(SharedObj)
ServerObj:= $(ServerSrc:src/server/%.cpp=obj/server/%.o) $(SharedObj)

.PHONY: build

build:
	@make server -j8 --silent
	@make client -j8 --silent

obj: ; @mkdir obj
obj/server/ obj/client/ obj/shared/: obj
	@echo "-- Making build directory: $@ --"
	@mkdir $@

server: $(ServerObj)
	@echo "-- Linking Server --"
	@$(GCC) $(ServerObj) $(ServerLFlags) -oserver

client: $(ClientObj)
	@echo "-- Linking Client --"
	@$(GCC) $(ClientObj) $(ClientLFlags) -oclient

src/shared/block.cpp: include/shared/blocks/*.h
	@touch src/shared/block.cpp

obj/server/%.o: src/server/%.cpp include/server/%.h obj/server/
	@echo "-- Generating $@ --"
	@$(GCC) $(ServerSFlags) -c $< -o $@

obj/server/%.o: src/server/%.cpp obj/server/
	@echo "-- Generating $@ --"
	@$(GCC) $(ServerSFlags) -c $< -o $@

obj/client/%.o: src/client/%.cpp include/client/%.h obj/client/
	@echo "-- Generating $@ --"
	@$(GCC) $(ClientSFlags) -c $< -o $@

obj/client/%.o: src/client/%.cpp obj/client/
	@echo "-- Generating $@ --"
	@$(GCC) $(ClientSFlags) -c $< -o $@

obj/shared/%.o: src/shared/%.cpp include/shared/%.h obj/shared/
	@echo "-- Generating $@ --"
	@$(GCC) $(SharedSFlags) -c $< -o $@

obj/shared/%.o: src/shared/%.cpp obj/shared/
	@echo "-- Generating $@ --"
	@$(GCC) $(SharedSFlags) -c $< -o $@

run: build
	@echo "-- Running --"
	@./client

clean:
	@echo "-- Cleaning --"
	@rm -rf obj/
	@rm -f client server
	