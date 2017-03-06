########
#   Directories
S_DIR=Source
B_DIR=Build
GLMDIR=glm

# Name of source file
OBJECTS = $(B_DIR)/main.o $(B_DIR)/helpers.o

########
#   Output
EXEC=$(B_DIR)/rasterizer

# default build settings # Can add -O3 to add optimisations
CC_OPTS=-c -std=c++11 -fopenmp -pipe -Wall -pedantic -Wextra -O3 -Wno-switch -ggdb -g3 -lpthread
LN_OPTS=-fopenmp
CC=g++

########
#       SDL options
SDL_CFLAGS := $(shell sdl-config --cflags)
GLM_CFLAGS := -I$(GLMDIR)
SDL_LDFLAGS := $(shell sdl-config --libs)

########
#   This is the default action
all: clean / Build
	$(EXEC)

########
#   Objects
#$(S_DIR)/SDLauxiliary.h $(S_DIR)/TestModel.h
$(B_DIR)/%.o : $(S_DIR)/%.cpp
	$(CC) $(CC_OPTS) -c $< -o $@ $(SDL_CFLAGS) $(GLM_CFLAGS)


########
#   Main build rule
Build: create $(OBJECTS) Makefile
	$(CC) $(LN_OPTS) -o $(EXEC) $(OBJECTS) $(SDL_LDFLAGS)

create:
	mkdir -p $(B_DIR)

clean:
	rm -rf $(B_DIR)
