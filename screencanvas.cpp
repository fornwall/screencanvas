#include "screencanvas.hpp"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

static struct winsize global_termsize;
static bool global_termsize_changed = false;

static void sigwinch_handler(int)
{
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, (char*) &global_termsize) < 0) {
                perror("ioctl() failed");
                exit(1);
        }
        global_termsize_changed = true;
}


Terminal::Terminal() {
        struct winsize termSize;
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, (char*) &termSize) < 0) {
                perror("ioctl() failed");
                exit(1);
        }
        mRows = termSize.ws_row;
        mColumns = termSize.ws_col;

        tcgetattr(0, &vt_orig); /* get the current settings */
        struct termios trm = vt_orig;
        trm.c_cc[VMIN] = 1; 	// Minimum number of characters for noncanonical read (MIN).
        trm.c_cc[VTIME] = 0; 	// Timeout in deciseconds for noncanonical read (TIME).
        // echo off, canonical mode off, extended input processing off, signal chars off:
        trm.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        tcsetattr(STDIN_FILENO, TCSANOW, &trm);

        struct sigaction newSigAction;
        newSigAction.sa_handler = sigwinch_handler;
        newSigAction.sa_mask = 0;
        newSigAction.sa_flags = 0;
        struct sigaction oldSigAction;
        if (sigaction(SIGWINCH, &newSigAction, &oldSigAction) < 0) {
                perror("sigaction() failed");
                exit(1);
        }

}

Terminal::~Terminal() {
        tcsetattr(0, TCSANOW, &vt_orig);
        if (cursor_app_set) printf("\033[?1l");
        if (keypad_app_set) printf("\033[?66l"); // CSI ? 1 h, Application Cursor Keys
        if (bracketed_paste_mode) printf("\033[?2004l");
        if (cursor_hidden) showCursor();
        if (mouse_enabled) disableMouse();
        setForeground(Color::DEFAULT).setBackground(Color::DEFAULT);
        if (alt_screen_set) leaveAltScreen();
        fflush(stdout);
}

EventType Terminal::await() {
        mLastEventType = EventType::NONE;
        while (mLastEventType == EventType::NONE) {
                if (mReadBufferLength == 0) {
                        int bytesRead = read(STDIN_FILENO, mReadBuffer + mReadBufferOffset, sizeof(mReadBuffer) - mReadBufferOffset - mReadBufferLength);
                        if (bytesRead == 0) {
                                perror("read(0");
                                exit(1);
                        } else if (bytesRead < 0) {
                                if (errno == EINTR && global_termsize_changed) {
                                        mLastEventType = EventType::RESIZE;
                                        mRows = global_termsize.ws_row;
                                        mColumns = global_termsize.ws_col;
                                        global_termsize_changed = false;
                                } else {
                                        print("READ SIGNAL");
                                        perror("read()");
                                        exit(1);
                                }
                        } else {
                                mReadBufferLength += bytesRead;
                        }
                }

                while (mLastEventType == EventType::NONE && mReadBufferLength > 0) {
                        processByte(mReadBuffer[mReadBufferOffset]);
                        if (--mReadBufferLength == 0) {
                                mReadBufferOffset = 0;
                        } else {
                                mReadBufferOffset++;
                        }
                }
        }
        return mLastEventType;
}

Terminal& Terminal::moveCursor(int right, int up) {
        // CSI Ps A Cursor Up Ps Times (default = 1) (CUU).
        // CSI Ps B Cursor Down Ps Times (default = 1) (CUD).
        // CSI Ps C Cursor Forward Ps Times (default = 1) (CUF).
        // CSI Ps D Cursor Backward Ps Times (default = 1) (CUB).
        if (up != 0) printf("\033[%d%c", up > 0 ? up : -up, up > 0 ? 'A' : 'B');
        if (right != 0) printf("\033[%d%c", right > 0 ? right : -right, right > 0 ? 'C' : 'D');
        fflush(stdout);
        return *this;
}

void Terminal::fillRectangle(unsigned int left, unsigned int top, unsigned int right, unsigned int bottom, uint32_t codepoint) {
        clipToScreen(top, left);
        clipToScreen(bottom, right);
        if (bottom >= top || left >= right) return;
        print("\033[%d;%d;%d;%d;%d$x", codepoint, bottom, left, top, right);
}

void Terminal::processByte(uint8_t byte) {
        switch (mEscapeState) {
                case EscapeState::NONE:
                        if (byte == 27) {
                                mEscapeState = EscapeState::ESCAPE;
                        } else {
                                if (mUtf8Remaining > 0) {
                                        if ((byte & 0b11000000) != 0b10000000) {
                                                // Not a UTF-8 continuation byte so replace the entire sequence up to now with the replacement char:
                                                mLastCharacter = 0xFFFD;
                                                mLastEventType = EventType::CHAR;
                                        } else {
                                                mUtf8Buffer[mUtf8Index++] = byte;
                                                if (--mUtf8Remaining == 0) {
                                                        uint8_t firstByteMask = (uint8_t) (mUtf8Index == 2 ? 0b00011111 : (mUtf8Index == 3 ? 0b00001111 : 0b00000111));
                                                        uint32_t codePoint = (mUtf8Buffer[0] & firstByteMask);
                                                        for (unsigned int i = 1; i < mUtf8Index; i++)
                                                                codePoint = ((codePoint << 6) | (mUtf8Buffer[i] & 0b00111111));
                                                        if (((codePoint <= 0b1111111) && mUtf8Index > 1) || (codePoint < 0b11111111111 && mUtf8Index > 2)
                                                                        || (codePoint < 0b1111111111111111 && mUtf8Index > 3)) {
                                                                // Overlong encoding.
                                                                codePoint = 0xFFFD;
                                                        }

                                                        mUtf8Index = mUtf8Remaining = 0;

                                                        if (codePoint >= 0x80 && codePoint <= 0x9F) {
                                                                // Sequence decoded to a C1 control character which is
                                                                // the same as ESC followed by ((code & 0x7f) + 0x40).
                                                                // processCodePoint(27);
                                                                // FIXME
                                                                // processCodePoint((codePoint & 0x7F) + 0x40);
                                                        } else {
                                                                //if (Character.UNASSIGNED == Character.getType(codePoint)) codePoint = UNICODE_REPLACEMENT_CHAR;
                                                                mLastCharacter = 0xFFFD;
                                                                mLastEventType = EventType::CHAR;
                                                        }
                                                }
                                        }
                                } else {
                                        if ((byte & 0b10000000) == 0) { // The leading bit is not set so it is a 7-bit ASCII character.
                                                mLastCharacter = byte;
                                                mLastEventType = EventType::CHAR;
                                        } else if ((byte & 0b11100000) == 0b11000000) { // 110xxxxx, a two-byte sequence.
                                                mUtf8Remaining = 1;
                                        } else if ((byte & 0b11110000) == 0b11100000) { // 1110xxxx, a three-byte sequence.
                                                mUtf8Remaining = 2;
                                        } else if ((byte & 0b11111000) == 0b11110000) { // 11110xxx, a four-byte sequence.
                                                mUtf8Remaining = 3;
                                        } 
                                        mUtf8Buffer[mUtf8Index++] = byte;
                                }
                        }
                        break;
                case EscapeState::ESCAPE:
                        switch (byte) {
                                case '[':
                                        mEscapeState = EscapeState::CSI;
                                        break;
                                case 'O':
                                        mEscapeState = EscapeState::ESCAPE_O;
                                        break;
                                default:
                                        escapeError("invalid input in ESCAPE");
                                        break;
                        }
                        break;
                case EscapeState::CSI:
                        switch (byte) {
                                case 'A':
                                case 'B':
                                case 'C':
                                case 'D':
                                        mLastKey = Key(uint32_t(Key::UP) + (byte - 'A'));
                                        mLastEventType = EventType::KEY;
                                        escapeDone();
                                        break;
                                case '<':
                                        mEscapeState = EscapeState::CSI_LOWERTHAN;
                                        break;
                                case '~':
                                        if (mCurrentEscapeArg != 0) {
                                                escapeError("Found ~ after CSI, but num args=%d", mCurrentEscapeArg);
                                        } else {
                                                switch (argAsNumber(0)) {
                                                        case 15: mLastKey = Key::F5; break;
                                                        case 17: mLastKey = Key::F6; break;
                                                        case 18: mLastKey = Key::F7; break;
                                                        case 19: mLastKey = Key::F8; break;
                                                        case 20: mLastKey = Key::F9; break;
                                                        case 21: mLastKey = Key::F10; break;
                                                        case 23: mLastKey = Key::F11; break;
                                                        case 24: mLastKey = Key::F12; break;
                                                        default: escapeError("Unknown CSI ~ number"); return;
                                                }
                                                mLastEventType = EventType::KEY;
                                                escapeDone();
                                        }
                                        break;
                                default:
                                        addEscapeArg(byte);
                        }
                        break;
                case EscapeState::CSI_LOWERTHAN:
                        switch (byte) {
                                case 'm':
                                case 'M':
                                        if (mCurrentEscapeArg != 2) {
                                                escapeError("MOUSE: %d, %c", mCurrentEscapeArg, byte);
                                        } else {
                                                mLastMouseColumn = argAsNumber(1) - 1;
                                                mLastMouseRow = flipRow(argAsNumber(2));
                                                mLastEventType = (byte == 'M') ? EventType::MOUSE_DOWN : EventType::MOUSE_UP;
                                                escapeDone();
                                        }
                                        break;
                                default:
                                        addEscapeArg(byte);
                                        break;
                        }
                        break;
                case EscapeState::ESCAPE_O:
                        switch (byte) {
                                case 'P': mLastKey = Key::F1; break;
                                case 'Q': mLastKey = Key::F2; break;
                                case 'R': mLastKey = Key::F3; break;
                                case 'S': mLastKey = Key::F4; break;
                                default: escapeError("ESCAPE_O followed by strange char"); return;
                        }
                        mLastEventType = EventType::KEY;
                        escapeDone();
                        break;
        }
}

Terminal& Terminal::print(char const* fmt, ...) {
        va_list argp;
        va_start(argp, fmt);
        vfprintf(stdout, fmt, argp);
        va_end(argp);
        fflush(stdout);
        return *this;
}

Terminal& Terminal::setTitle(char const* fmt, ...) { 
        print("\033]0;");
        va_list argp;
        va_start(argp, fmt);
        vfprintf(stdout, fmt, argp);
        va_end(argp);
        fflush(stdout);
        print("\007");
        return *this;
}
