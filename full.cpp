#include "screencanvas.hpp"

int main() {
        Terminal term;
        term.enterAltScreen().enableMouse().placeCursor(50, 50);
        term
                //.setForeground(Color::BLACK)
                //.setBackground(Color::GREEN)
                .clear();
        term.fillRectangle(50, 8, 60, 10, 'X');
        term.placeCursor(term.columns() / 2, term.rows() - 1).print("[HELLO WORLD TOP]");
        term.placeCursor(term.columns() / 2, 0).print("[HELLO WORLD BOTTOM]");
        term.placeCursor(5, 5);
        term.print("Hello %d, %d", term.rows(), term.columns());
        bool run = true;
        while (run) {
                switch (term.await()) {
                        case EventType::KEY:
                                switch (term.lastKey()) {
                                        case Key::UP: term.moveCursor(0, 1); break;
                                        case Key::DOWN: term.moveCursor(0, -1); break;
                                        case Key::LEFT: term.moveCursor(-1, 0); break;
                                        case Key::RIGHT: term.moveCursor(1, 0); break;
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
                                       if (c == 'A') term.placeCursorAtColumn(term.columns() - 1);
                                       else if (c == '|') term.setCursorStyleBar();
                                       else if (c == 'B') term.setCursorStyleBlock();
                                       else if (c == 'C') term.clear().placeCursor(40, 5).setTitle("CLEARED");
                                       else if (c == 'D') term.deleteLines(2);
                                       else if (c == 'U') term.clear()
                                                          .placeCursor(5, 5).print("Just one combining:  \u0302")
                                                          .placeCursor(5, 6).print("'o' followed by combining: o\u0302")
                                                          .placeCursor(5, 7).print("'o' followed by combining, then go back and replace: o\u0302").moveCursor(-1,0).print("U")
                                                          .placeCursor(5, 8).print("'o' last column followed by combining:").placeCursor(term.columns() - 1, 8).print("o\u0302")
                                                          .placeCursor(5,  9).print("'ou', then back and combining: ou").moveCursor(-1, 0).print("\u0302")
                                                          .placeCursor(5, 10).print("Double-width and combining: 枝").print("\u0302")
                                                          .placeCursor(5, 11).print("Double, back 1 and combining: 枝").moveCursor(-1, 0).print("\u0302")
                                                          .placeCursor(5, 12).print("Double, back 2 and combining: 枝").moveCursor(-2, 0).print("\u0302")
                                                          .placeCursor(5, 13).print("枝").placeCursorAtColumn(45).placeCursorAtColumn(41).print("\u0302")
                                                          .placeCursor(5, 14).print("Double-width, back 2 and a: '枝'").moveCursor(-3, 0).print("a")
                                                          .placeCursor(5, 15).print("Double-width, back 1 and a: '枝'").moveCursor(-2, 0).print("a")
                                                          .placeCursor(5, 16).print("Double-width in last col:").placeCursorAtColumn(term.columns()-1).print("枝")
                                                          .placeCursor(5, 17).print("Single-width in last col:").placeCursorAtColumn(term.columns()-1).print("a")
                                                          .placeCursor(5, 18).print("Combining in first col <=").placeCursorAtColumn(0).print("\u0302")
                                                          .placeCursor(5, 19).print("a to last column and combining:").placeCursorAtColumn(term.columns()-1).print("a\u0302")
                                                          .placeCursor(5, 20);
                                       else if (c == 'R') term.resetColorsAndStyle();
                                       else if (c >= '0' && c <= '9') term.setBackground(Color(int(Color::BLACK) + (c - '0')));
                                       else if (c == 'F') term.print("枝");
                                       else if (c == 'I') term.placeCursorAtColumn(0);
                                       else if (c == 'L') term.insertLines(2);
                                       else if (c == 'Z') term.print("\u0302");
                                       else if (c == '_') term.setCursorStyleUnderline();
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
                               //term.fillRectangle(term.lastMouseColumn(), term.lastMouseRow(), term.lastMouseColumn()+10, term.lastMouseRow()+3, 'X');
                               break;
                       default:
                               break;
               }
        }
        return 0;
}
