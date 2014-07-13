#ifndef TERM_HPP_INCLUDED
#define TERM_HPP_INCLUDED

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>

enum class EventType : uint8_t { KEY, MOUSE_DOWN, MOUSE_UP, CHAR, RESIZE, TIMEOUT, PASTE, NONE };
enum class Key : uint16_t { UP, DOWN, RIGHT, LEFT, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12 };
enum class ModifierKey : uint8_t { CTRL, SHIFT, ALT };
enum class Color : uint16_t { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, DEFAULT=9 };
struct Size { unsigned int rows, columns; };
enum class EscapeState { NONE, ESCAPE, ESCAPE_O, CSI, CSI_LOWERTHAN };

#define WARN_UNUSED __attribute__((warn_unused_result))

class Terminal {
        private:
                /** Coordinate system is 0,0 in lower left corner ranging to (cols-1, rows-1). */
                unsigned int WARN_UNUSED flipRow(unsigned int row) const { return mRows - row; }
	public:
		Terminal();
		~Terminal();

		void setCursorApp() { this->cursor_app_set = true; print("\033[?1h"); }
		void setKeypadApp() { this->keypad_app_set = true; print("\033[66?h"); }

                void setBracketedPasteMode(bool set) { print("\033[?2004%s", set ? "h" : "l"); this->bracketed_paste_mode = set; }

                Terminal& enterAltScreen() { alt_screen_set = true; decPrivateMode(1049, true); return *this; }
                Terminal& leaveAltScreen() { alt_screen_set = false; decPrivateMode(1049, false); return *this;  }

                /** Discard the latest event and read another one. Does not need to read from the terminal if buffered. */
                EventType await();

                Terminal& sleep(unsigned int milliseconds) { usleep(milliseconds * 1000); return *this; }
                Terminal& clear() { print("\033[2J"); /* ED – Erase In Display */; return *this; }

                Terminal& setBackground(Color color) { print("\033[%dm", 40 + int(color)); return *this; }
                Terminal& setForeground(Color color) { print("\033[%dm", 30 + int(color)); return *this; }
                Terminal& resetColorsAndStyle() { print("\033[0m"); return *this; }
                Terminal& setTitle(char const* fmt, ...);

                Terminal& showCursor() { cursor_hidden = false; decPrivateMode(25, true); return *this; }
                Terminal& hideCursor() { cursor_hidden = true; decPrivateMode(25, false); return *this; }
		Terminal& placeCursor(unsigned int x, unsigned int y) { print("\033[%u;%uH", flipRow(y), x + 1); return *this; }
		Terminal& placeCursorAtColumn(unsigned int x) { print("\033[%uG", x + 1); return *this; }
                Terminal& moveCursor(int up, int right);

                Terminal& setCursorStyleBlock() { print("\033[2 q"); return *this; }
                Terminal& setCursorStyleUnderline() { print("\033[4 q"); return *this; }
                Terminal& setCursorStyleBar() { print("\033[6 q"); return *this; }

                Terminal& enableMouse() { mouse_enabled = true; decPrivateMode(1000, true); decPrivateMode(1006, true); return *this; }
                Terminal& disableMouse() { mouse_enabled = false; decPrivateMode(1000, false); decPrivateMode(1006, false); return *this; }

                Terminal& insertLines(unsigned int lines) { print("\033[%dL", lines); return *this; }
                Terminal& deleteLines(unsigned int lines) { print("\033[%dM", lines); return *this; }

                Terminal& print(char const* fmt, ...);
                uint32_t columns() const { return mColumns; }
                uint32_t rows() const { return mRows; }

                bool modifierControl() const { return (mLastModifierKeys & (1 << int(ModifierKey::CTRL))) != 0; }
                bool modifierShift() const { return (mLastModifierKeys & (1 << int(ModifierKey::SHIFT))) != 0; }

                void fillRectangle(unsigned int column, unsigned int row, unsigned int columns, unsigned int rows, uint32_t codepoint);
                uint32_t lastCharacter() const { return mLastCharacter; }
                Key lastKey() const { return mLastKey; }

                unsigned int lastMouseRow() const { return mLastMouseRow; }
                unsigned int lastMouseColumn() const { return mLastMouseColumn; }

        private:
                uint8_t mReadBuffer[32];
                unsigned int mReadBufferLength{0};
                unsigned int mReadBufferOffset{0};
                uint8_t mUtf8Buffer[4];
                unsigned int mUtf8Index{0};
                unsigned int mUtf8Remaining{0};
                bool const is_tty{!!isatty(0)};
		struct termios vt_orig;
		bool cursor_app_set{false};
		bool keypad_app_set{false};
                bool bracketed_paste_mode{false};
                bool alt_screen_set{false};
                bool mouse_enabled{false};
                bool cursor_hidden{false};

                uint32_t mRows{0};
                uint32_t mColumns{0};

                Key mLastKey{Key::UP};
                uint32_t mLastModifierKeys{0};
                EventType mLastEventType{EventType::NONE};
                uint32_t mLastCharacter{0};

                static const unsigned int MAX_ESCAPE_ARGS = 32;
                static const unsigned int MAX_ESCAPE_ARG_LENGTH = 128;
                char mEscapeArguments[MAX_ESCAPE_ARGS][MAX_ESCAPE_ARG_LENGTH];
                unsigned int mCurrentEscapeArg{0};
                unsigned int mCurrentEscapeArgLengths[MAX_ESCAPE_ARGS]{0};
                unsigned int mLastMouseRow;
                unsigned int mLastMouseColumn;

                EscapeState mEscapeState{EscapeState::NONE};

                void clipToScreen(unsigned int& row, unsigned int col) const {  row = flipRow(row); if (row >= mRows) row = mRows - 1; if (col >= mColumns) col = mColumns - 1; }
                void escapeError(char const* fmt, ...) { 
                        va_list argp;
                        va_start(argp, fmt);
                        vfprintf(stderr, fmt, argp); 
                        va_end(argp);
                        fflush(stderr);
                        escapeDone();
                };

                void escapeDone() {
                        mEscapeState = EscapeState::NONE;
                        mCurrentEscapeArg = 0;
                        memset(mEscapeArguments, 0, sizeof(mEscapeArguments));
                        memset(mCurrentEscapeArgLengths, 0, sizeof(mCurrentEscapeArgLengths));
                }

                void processByte(uint8_t byte);

                void addEscapeArg(uint8_t byte) { 
                        if (byte == ';') {
                                if (mCurrentEscapeArg + 1 == MAX_ESCAPE_ARGS) {
                                        escapeError("Too man escape args");
                                } else {
                                        mCurrentEscapeArg++;
                                }
                        } else {
                                unsigned int& len = mCurrentEscapeArgLengths[mCurrentEscapeArg];
                                if (len + 1 == MAX_ESCAPE_ARG_LENGTH) {
                                        escapeError("Too larg escape sequence");
                                } else {
                                        mEscapeArguments[mCurrentEscapeArg][len++] = byte;
                                }
                        }
                }

                int argAsNumber(unsigned int index) {
                        if (index > mCurrentEscapeArg) {
                                perror("argAsNumber ERROR");
                                _exit(1);
                        }
                        mEscapeArguments[index][mCurrentEscapeArgLengths[index]] = 0;
                        return atol(mEscapeArguments[index]);
                }

		void decPrivateMode(unsigned int mode, bool set) {
			printf("\033[?%u%c", mode, (set ? 'h' : 'l'));
			fflush(stdout);
		}
};

#endif
