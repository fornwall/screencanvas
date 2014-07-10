#include "screencanvas.hpp"

int main() {
        Terminal term;
        term.placeCursor(50, 50);
        term.sleep(1000);
        term.placeCursor(5, 5);
        return 0;
}
