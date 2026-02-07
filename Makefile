TARGET = vk_template
SRCS = main.c util.c
INC_DIRS = -I./external/cglm/include
CFLAGS = -Wall -Wextra -ggdb
LINK_LIBS = -lglfw -lvulkan
DEBUG = -DDEBUG

all: sources shader

sources: $(SRCS)
	cc -o $(TARGET) $(SRCS) $(CFLAGS) $(INC_DIRS) $(LINK_LIBS) $(DEBUG)

shader: shaders/shader.*
	glslc shaders/shader.vert -o shaders/vert.spv
	glslc shaders/shader.frag -o shaders/frag.spv

.PHONY: clean
clean:
	rm *.o vk_template *~ shaders/*spv
