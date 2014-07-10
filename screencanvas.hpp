#ifndef TERM_HPP_INCLUDED
#define TERM_HPP_INCLUDED

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>

enum class EventType : uint8_t { KEY, CHAR, RESIZE, TIMEOUT, NONE };
enum class Key : uint16_t { UP = 0, DOWN, RIGHT, LEFT, F1, F2, F3, F4 };
enum class ModifierKey : uint8_t { CTRL, SHIFT, ALT };

enum class Color : uint16_t { BLACK = 0, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, DEFAULT };

struct Size { unsigned int rows, columns; };

enum class EscapeState { NONE, ESCAPE, CSI };

class Character {
};

class KeyEvent {
        public:
                uint32_t getValue() const { return value; }
        private:
                uint32_t value;
};


class InputEvent {
        public:
                uint32_t getValue() const { return value; }
        private:
                uint32_t value;
};

/*
struct BackgroundColorOutput {
        ForegroundColorSinc& operator<<(Color color) { 
                printf("\033[%dm");
                return *this;
        };
};
*/

class Terminal {
        private:
                /** Coordinate system is 0,0 in lower left corner ranging to (cols-1, rows-1) */
                unsigned int flipRow(unsigned int row) const { return mRows - row - 1; }
	public:
		Terminal();
		~Terminal();

		void setCursorApp() {
			this->cursor_app_set = true;
			printf("\033[?1h"); // CSI ? 1 h, Application Cursor Keys
			fflush(stdout);
		}

		void setKeypadApp() {
			this->keypad_app_set = true;
			//printf("\033[?66h"); // CSI ? 1 h, Application Cursor Keys
			printf("\033="); // CSI ? 1 h, Application Cursor Keys
			fflush(stdout);
		}

		Terminal& placeCursor(unsigned int x, unsigned int y) { print("\033[%u;%uH", flipRow(y), x); return *this; }

                void setBracketedPasteMode(bool set) {
                        if (set == this->bracketed_paste_mode) return;
                        printf("\033[?2004%s", set ? "h" : "l");
                        fflush(stdout);
                        this->bracketed_paste_mode = set;
                }

                Terminal& enterAltScreen() { alt_screen_set = true; decPrivateMode(1049, true); return *this; }
                Terminal& leaveAltScreen() { alt_screen_set = false; decPrivateMode(1049, false); return *this;  }

                EventType await();

                Terminal& sleep(unsigned int milliseconds) { usleep(milliseconds * 1000); return *this; }
                Terminal& clear() { print("\033[2J"); /* ED – Erase In Display */; return *this; }

                Terminal& setBackground(Color color) { printf("\033[%dm", 40 + int(color)); fflush(stdout); return *this; }
                Terminal& setForeground(Color color) { printf("\033[%dm", 30 + int(color)); fflush(stdout); return *this; }

                Terminal& showCursor() { cursor_hidden = false; decPrivateMode(25, true); return *this; }
                Terminal& hideCursor() { cursor_hidden = true; decPrivateMode(25, false); return *this; }

                Terminal& moveCursor(int rowsDown, int columnsRight);

                void print(char const* fmt, ...) {
                        va_list argp;
                        va_start(argp, fmt);
                        vfprintf(stderr, fmt, argp);
                        va_end(argp);
                        fflush(stdout);
                };
                uint32_t columns() const { return mColumns; }
                uint32_t rows() const { return mRows; }

                void fillRectangle(unsigned int column, unsigned int row, unsigned int columns, unsigned int rows, uint32_t codepoint);
                //Size size() const { Size s; s.rows = rows; s.columns = columns; return s; }
                uint32_t eatChar();
                Key lastKey() const { return mLastKey; }

        private:
                uint8_t mReadBuffer[32];
                unsigned int mReadBufferLength{0};
                unsigned int mReadBufferOffset{0};
                bool const is_tty{!!isatty(0)};
		struct termios vt_orig;
		bool cursor_app_set{false};
		bool keypad_app_set{false};
                bool bracketed_paste_mode{false};
                bool alt_screen_set{false};
                bool cursor_hidden{false};

                uint32_t mRows{0};
                uint32_t mColumns{0};

                Key mLastKey{Key::UP};
                EscapeState mEscapeState{EscapeState::NONE};
                EventType mLastEventType;

                void clipToScreen(unsigned int& row, unsigned int col) const {  row = flipRow(row); if (row >= mRows) row = mRows - 1; if (col >= mColumns) col = mColumns - 1; }
                void abortEscapeState(char const*) { mEscapeState = EscapeState::NONE; }
                void processByte(uint8_t byte);

		void decPrivateMode(unsigned int mode, bool set) {
			printf("\033[?%u%c", mode, (set ? 'h' : 'l'));
			fflush(stdout);
		}
};

#endif
