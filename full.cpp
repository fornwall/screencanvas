#include "screencanvas.hpp"

int main() {

        Terminal term;
        term.enterAltScreen().enableMouse().placeCursor(50, 50);
        term.placeCursor(term.columns() / 2, term.rows() - 1).print("[HELLO WORLD]");
        term.placeCursor(5, 5);
        term.setForeground(Color::BLACK).setBackground(Color::GREEN).clear();
        term.fillRectangle(50, 8, 60, 10, 'X');
        term.print("Hello %d, %d", term.rows(), term.columns());
        bool run = true;
        while (run) {
                switch (term.await()) {
                        case EventType::KEY:
                                switch (term.lastKey()) {
                                        case Key::UP: term.moveCursor(1, 0); break;
                                        case Key::DOWN: term.moveCursor(-1, 0); break;
                                        case Key::LEFT: term.moveCursor(0, -1); break;
                                        case Key::RIGHT: term.moveCursor(0, 1); break;
                                        default: break;
                                }
                                break;
                       case EventType::CHAR:
                               {
                                       uint32_t c = term.lastCharacter();
                                       if (c == 3 || c == 4) run = false;
                                       if (c == 'c') term.clear().placeCursor(40, 5).print("CLEARED!");
                                       else if (c >= '0' && c <= '9') term.setBackground(Color(int(Color::BLACK) + (c - '0'))).clear();
                                       else term.print("%c",(char) c);
                               }
                               break;
                       case EventType::RESIZE:
                               term.setTitle("RESIZE %d,%d", term.columns(), term.rows()).clear().placeCursor(term.columns()/2, 0);
                               break;
                       case EventType::TIMEOUT:
                               term.print("TIMEOUT ");
                               break;
                       case EventType::MOUSE_DOWN:
                       case EventType::MOUSE_UP:
                               term.placeCursor(term.lastMouseColumn(), term.lastMouseRow());
                               break;
                       default:
                               break;
               }
        }
        //term.hideCursor();
        //term.sleep(2000);
        return 0;
}
