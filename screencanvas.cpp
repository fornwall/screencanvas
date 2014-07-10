#include "screencanvas.hpp"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

static void sigwinch_handler(int)
{
        struct winsize termSize;
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, (char*) &termSize) < 0) {
                perror("ioctl() failed");
                exit(1);
        }
        printf("screen size changed\n");
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
        if (alt_screen_set) leaveAltScreen();
        fflush(stdout);
};

EventType Terminal::await() {
        if (mReadBufferLength > mReadBufferOffset) return EventType::CHAR;

        int bytesReadNow = read(STDIN_FILENO, mReadBuffer + mReadBufferOffset, sizeof(mReadBuffer) - mReadBufferOffset - mReadBufferLength);
        if (bytesReadNow == 0) {
                perror("read(0");
                exit(1);
        } else if (bytesReadNow < 0) {
                perror("read()");
                exit(1);
        } else {
                mReadBufferLength += bytesReadNow;
                return EventType::CHAR;
        }
        return EventType::RESIZE;
}

Terminal& Terminal::moveCursor(int rowsDown, int columnsRight) {
        // CSI Ps A Cursor Up Ps Times (default = 1) (CUU).
        // CSI Ps B Cursor Down Ps Times (default = 1) (CUD).
        // CSI Ps C Cursor Forward Ps Times (default = 1) (CUF).
        // CSI Ps D Cursor Backward Ps Times (default = 1) (CUB).
        if (rowsDown != 0) printf("\033[%d%c", rowsDown < 0 ? -rowsDown : rowsDown, rowsDown < 0 ? 'A' : 'B');
        if (rowsDown != 0) printf("\033[%d%c", columnsRight < 0 ? -columnsRight : columnsRight, columnsRight < 0 ? 'C' : 'D');
        fflush(stdout);
        return *this;
};

void Terminal::fillRectangle(unsigned int left, unsigned int top, unsigned int right, unsigned int bottom, uint32_t codepoint) {
        clipToScreen(top, left);
        clipToScreen(bottom, right);
        if (bottom >= top || left >= right) return;
        print("ARG=%d;%d;%d;%d;%d$x", codepoint, bottom, left, top, right);
        print("\033[%d;%d;%d;%d;%d$x", codepoint, bottom, left, top, right);
}

uint32_t Terminal::eatChar() {
        if (mReadBufferLength == 0) return 0;
        uint32_t returnValue = mReadBuffer[mReadBufferOffset];
        if (--mReadBufferLength == 0) {
                mReadBufferOffset = 0;
        } else {
                mReadBufferOffset++;
        }
        return returnValue;

}

void Terminal::processByte(uint8_t byte) {
        bool leaveEscapeState = true;
        mLastEventType = EventType::NONE;
        switch (mEscapeState) {
                case EscapeState::NONE:
                        if (byte == 27) {
                                mEscapeState = EscapeState::ESCAPE;
                                leaveEscapeState = false;
                        }
                        break;
                case EscapeState::ESCAPE:
                        switch (byte) {
                                case '[':
                                        mEscapeState = EscapeState::CSI;
                                        break;
                                case 'A':
                                case 'B':
                                case 'C':
                                case 'D':
                                        mLastKey = Key(uint32_t(Key::UP) + (byte - 'A'));
                                        mLastEventType = EventType::KEY;
                                        break;
                                default:
                                        abortEscapeState("invalid input in ESCAPE");
                                        break;
                        }
                        break;
                case EscapeState::CSI:
                        abortEscapeState("CSI unhandled");
                        break;
        }
        if (leaveEscapeState) mEscapeState = EscapeState::NONE;
}
