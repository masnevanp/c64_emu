
TARGET := c64_emu

BUILD ?= debug
VERSION_INFO ?= unknown

CXXFLAGS := -std=c++11 -Wall -pedantic -Wextra -D__VERSION_INFO__=\"$(VERSION_INFO)\"
LDFLAGS :=

ifeq ($(BUILD), release)
CXXFLAGS += -DNDEBUG -O3
LDFLAGS += -s
else
CXXFLAGS += -DDEBUG -O0 -g
endif

SDL_LIBS := `sdl2-config --libs`
SDL_CFLAGS := `sdl2-config --cflags`


SRC := $(wildcard *.cpp nmos6502/*.cpp resid/*.cpp)
OBJ := $(SRC:%.cpp=obj/$(BUILD)/%.o)


.PHONY: all debug release run clean

all: bin/$(BUILD)/$(TARGET)

debug: all

release:
	@$(MAKE) all BUILD=release

bin/$(BUILD)/$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	@$(CXX) -o $@ $(LDFLAGS) $(SDL_LIBS) $^
	@echo "  ==> $@"

obj/$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) $< -c -o $@
	@echo "$< --> $@"

run:
	@./bin/$(BUILD)/$(TARGET)

run_release:
	@$(MAKE) run BUILD=release

clean:
	@rm -fr obj
	@rm -fr bin

