LIBS     = -lGL -lglfw -ldl
HEADERS  = shaderprogram.h
FILES = main_file.cpp shaderprogram.cpp lodepng.cpp model.cpp objmodel.cpp
GLAD_SRC = glad/src/gl.c

main_file: $(FILES) $(HEADERS) $(GLAD_SRC)
	g++ -o main_file $(FILES) $(GLAD_SRC) $(LIBS) -I./glm/ -I./glad/include/

clean:
	rm -f main_file
