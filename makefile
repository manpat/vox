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
obj/server obj/client obj/shared obj/client/gui: obj
	@echo "-- Checking build directory: $@ --"
	@$(shell [ ! -d $@ ] && mkdir $@)

server: $(ServerObj)
	@echo "-- Linking Server --"
	@$(GCC) $(ServerObj) $(ServerLFlags) -oserver

client: $(ClientObj)
	@echo "-- Linking Client --"
	@$(GCC) $(ClientObj) $(ClientLFlags) -oclient

src/shared/block.cpp: include/shared/blocks/*.h
	@touch src/shared/block.cpp

src/client/gui/%.cpp: obj/client obj/client/gui ;

src/server/%.cpp: obj/server ;
src/shared/%.cpp: obj/shared ;
src/client/%.cpp: obj/client ;

obj/server/%.o: src/server/%.cpp include/server/%.h
	@echo "-- Generating $@ --"
	@$(GCC) $(ServerSFlags) -c $< -o $@

obj/server/%.o: src/server/%.cpp
	@echo "-- Generating $@ --"
	@$(GCC) $(ServerSFlags) -c $< -o $@

obj/client/%.o: src/client/%.cpp include/client/%.h
	@echo "-- Generating $@ --"
	@$(GCC) $(ClientSFlags) -c $< -o $@

obj/client/%.o: src/client/%.cpp
	@echo "-- Generating $@ --"
	@$(GCC) $(ClientSFlags) -c $< -o $@

obj/shared/%.o: src/shared/%.cpp include/shared/%.h
	@echo "-- Generating $@ --"
	@$(GCC) $(SharedSFlags) -c $< -o $@

obj/shared/%.o: src/shared/%.cpp
	@echo "-- Generating $@ --"
	@$(GCC) $(SharedSFlags) -c $< -o $@

run: build
	@echo "-- Running --"
	@gnome-terminal --geometry=120x24 -e "./server" &
	@./client &
	@./client &

	@xdotool search --sync --name "^Vox$$" windowactivate

clean:
	@echo "-- Cleaning --"
	@rm -rf obj/
	@rm -f client server
	