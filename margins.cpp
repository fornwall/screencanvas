#include "screencanvas.hpp"

int main() {
        Terminal term;
        term.placeCursor(0, 0);
        for (unsigned int i = 0; i < term.columns(); i++) term.print("X");
        term.placeCursor(0, term.rows()-1);
        for (unsigned int i = 0; i < term.columns(); i++) term.print("X");

        term.setMargins(2, term.rows()-1);

        //for (int i = 0; i < 200; i++) {
                //term.print("%d\n", i);
        //}
        //term.await();
        return 0;
}
