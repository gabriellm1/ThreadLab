all: uicp

uicp: uicp.o
	gcc -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL  libraylib.a uicp.c -o uicp
