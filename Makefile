src = $(wildcard *.cpp)
obj = $(src:.cpp=.o)
LDFLAGS = -lGL -lGLEW -lsfml-window -lsfml-graphics -lsfml-system

teapot: $(obj)
	g++ -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) teapot
