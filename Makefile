full: screencanvas.cpp full.cpp
	c++ $^ -o $@
alt: screencanvas.cpp alt.cpp
	c++ $^ -o $@
margins: screencanvas.cpp margins.cpp
	c++ $^ -o $@
demo: screencanvas.cpp demo.cpp
	c++ $^ -o $@

.PHONY: clean

clean:
	rm -f full demo alt margins
