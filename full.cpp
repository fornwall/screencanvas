#include "screencanvas.hpp"

int main() {
        Terminal term;
        term.enterAltScreen().placeCursor(50, 50);
        term.sleep(1000);
        term.placeCursor(5, 5);
        term.setForeground(Color::RED).setBackground(Color::BLUE);
        term.fillRectangle(50, 8, 60, 10, 'X');
        term.print("Hello %d, %d", term.rows(), term.columns());
        bool run = true;
        while (run) {
               switch (term.await()) {
                       case EventType::KEY:
                               term.print("KEY ");
                               break;
                       case EventType::CHAR:
                               {
                                       uint32_t c = term.eatChar();
                                       if (c == 3 || c == 4) run = false;
                                       if (c == 'c') term.clear().placeCursor(40, 5).print("CLEARED!");
                                       else if (c == 'd') term.print("hmm");
                                       else if (c >= '0' && c <= '9') term.setBackground(Color(int(Color::BLACK) + (c - '0'))).clear();
                                       else term.print("CHAR: %d ", c);
                               }
                               break;
                       case EventType::RESIZE:
                               term.print("RESIZE ");
                               break;
                       case EventType::TIMEOUT:
                               term.print("TIMEOUT ");
                               break;
               }
        }
        //term.hideCursor();
        //term.sleep(2000);
        return 0;
}
