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
                                        case Key::F1: term.setForeground(Color::RED).print("F1"); break;
                                        case Key::F2: term.setForeground(Color::GREEN).print("F2"); break;
                                        case Key::F3: term.setForeground(Color::BLUE).print("F3"); break;
                                        case Key::F4: term.setForeground(Color::YELLOW).print("F4"); break;
                                        case Key::F5: term.setForeground(Color::BLACK).print("F5"); break;
                                        case Key::F6: term.setForeground(Color::BLACK).print("F6"); break;
                                        case Key::F7: term.setForeground(Color::BLACK).print("F7"); break;
                                        case Key::F8: term.setForeground(Color::BLACK).print("F8"); break;
                                        case Key::F9: term.setForeground(Color::BLACK).print("F9"); break;
                                        case Key::F10: term.setForeground(Color::BLACK).print("F10"); break;
                                        case Key::F11: term.setForeground(Color::BLACK).print("F11"); break;
                                        case Key::F12: term.setForeground(Color::BLACK).print("F12"); break;
                                        default: break;
                                }
                                break;
                       case EventType::CHAR:
                               {
                                       uint32_t c = term.lastCharacter();
                                       if (c == 3 || c == 4) run = false;
                                       if (c == 'C') term.clear().placeCursor(40, 5).setTitle("CLEARED");
                                       else if (c == 'R') term.resetColorsAndStyle();
                                       else if (c >= '0' && c <= '9') term.setBackground(Color(int(Color::BLACK) + (c - '0')));
                                       else if (c == 'F') term.print("枝");
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
                               term.fillRectangle(term.lastMouseColumn(), term.lastMouseRow(), term.lastMouseColumn()+10, term.lastMouseRow()+3, 'X');
                               break;
                       default:
                               break;
               }
        }
        return 0;
}
