
TARGET := c64_emu

BUILD ?= debug
VERSION_INFO ?= unknown

CXXFLAGS := -std=c++17 -Wall -pedantic -Wextra -D__VERSION_INFO__=\"$(VERSION_INFO)\"
LDFLAGS :=

ifeq ($(BUILD), release)
CXXFLAGS += -DNDEBUG -O3
LDFLAGS += -s
else
CXXFLAGS += -DDEBUG -O0 -g
endif

SDL_LIBS := `pkg-config sdl3 --libs`
SDL_CFLAGS := `pkg-config sdl3 --cflags`


SRC := $(wildcard src/**.cpp src/*/*.cpp)
OBJ := $(SRC:src/%.cpp=obj/$(BUILD)/%.o)
DEP := $(OBJ:%.o=%.d)


.PHONY: all debug release run run_release clean

all: bin/$(BUILD)/$(TARGET)

debug: all

release:
	@$(MAKE) --silent all BUILD=release

bin/$(BUILD)/$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	@$(CXX) -o $@ $(LDFLAGS) $^ $(SDL_LIBS)
	@echo ;echo "    ==> $@"; echo

obj/$(BUILD)/%.o: src/%.cpp
	@echo -n "> $<"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) -MMD -MP $< -c -o $@
	@echo " --> $@"

run:
	@$(MAKE) --silent all
	@echo "Running: bin/$(BUILD)/$(TARGET)"
	@./bin/$(BUILD)/$(TARGET)

run_release:
	@$(MAKE) --silent run BUILD=release

clean:
	@rm -fr obj
	@rm -fr bin

-include $(DEP)
