#pragma once

#include <curses.h>

/**
 * @file
 * @brief The terminal, through curses: the demo's only "renderer" and
 *        "input". A service, given by the runtime (demo::install).
 *
 * Systems only draw and read keys when this service is there, so the same
 * systems can run without a terminal (in the editor's Play, for example):
 * the game logic runs, nothing is drawn.
 *
 * curses starts on first use, not when the service is created, so the
 * messages printed while the level loads stay readable.
 */
namespace demo {
    class Terminal {
        public:
            Terminal() = default;
            ~Terminal() {
                if (_started)
                    endwin();   // gives the terminal back as it was
            }
            Terminal(const Terminal&) = delete;
            Terminal& operator=(const Terminal&) = delete;

            /** @brief Starts curses (once): keys read one by one without waiting, no echo, no cursor. */
            void start() {
                if (_started)
                    return;
                initscr();
                cbreak();               // keys one by one; Ctrl+C still sends SIGINT
                noecho();
                keypad(stdscr, TRUE);   // arrow keys as KEY_UP...
                nodelay(stdscr, TRUE);  // getch() does not wait: the engine sets the pace
                curs_set(0);
                _started = true;
            }

            /** @brief The next key pressed, or ERR when there is none. */
            int readKey() {
                start();
                return getch();
            }

        private:
            bool _started = false;
    };
}
