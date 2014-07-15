#include "screencanvas.hpp"

int main() {
        Terminal term;
        term.placeCursor(10, 10).enterAltScreen();
        term.await();
        return 0;
}
