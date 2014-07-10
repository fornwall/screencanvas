full: screencanvas.cpp full.cpp
	c++ $^ -o $@

demo: screencanvas.cpp demo.cpp
	c++ $^ -o $@

.PHONY: clean

clean:
	rm -f full demo
