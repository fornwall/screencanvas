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
};

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

Terminal& Terminal::moveCursor(int up, int right) {
        // CSI Ps A Cursor Up Ps Times (default = 1) (CUU).
        // CSI Ps B Cursor Down Ps Times (default = 1) (CUD).
        // CSI Ps C Cursor Forward Ps Times (default = 1) (CUF).
        // CSI Ps D Cursor Backward Ps Times (default = 1) (CUB).
        if (up != 0) printf("\033[%d%c", up > 0 ? up : -up, up > 0 ? 'A' : 'B');
        if (right != 0) printf("\033[%d%c", right > 0 ? right : -right, right > 0 ? 'C' : 'D');
        fflush(stdout);
        return *this;
};

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
                                mLastCharacter = byte;
                                mLastEventType = EventType::CHAR;
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
                                                mLastMouseColumn = argAsNumber(1);
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
};

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
