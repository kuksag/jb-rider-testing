#include "ncurses.h"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

int main(int argc, char *argv[]) {
    // --------------------------------------------------------------------
    // extracting command line arguments

    std::string FILE_PATH = "words.txt";
    const std::size_t ALPHABET_SIZE = 'z' - 'a' + 1;
    std::size_t BOUND = 30;

    if (argc > 1) {
        if (argc != 3) throw std::runtime_error("Expected 0 or 2 arguments, got " + std::to_string(argc));
        char *number_end;
        BOUND = static_cast<std::size_t>(std::strtol(argv[1], &number_end, 10));
        FILE_PATH = argv[2];
    }

    // --------------------------------------------------------------------
    // extracting words from dictionary

    std::ifstream file(FILE_PATH, std::ifstream::in);
    if (!file.good()) throw std::runtime_error("Couldn't open file: " + FILE_PATH);

    std::vector<std::string> dictionary;
    std::vector<std::size_t> bundle[ALPHABET_SIZE][ALPHABET_SIZE]
                                   [ALPHABET_SIZE];

    for (std::string word; file >> word;) {
        auto check_letter = [](char ch) { return 'a' <= ch && ch <= 'z'; };
        for (std::size_t i = 0; i + 1 < word.size(); i++) {
            if (check_letter(word[i]) && check_letter(word[i + 1])) {
                if (i + 2 < word.size() && check_letter(word[i + 2])) {
                    bundle[word[i] - 'a'][word[i + 1] - 'a'][word[i + 2] - 'a'].push_back(dictionary.size());
                } else {
                    bundle[word[i] - 'a'][word[i + 1] - 'a'][0].push_back(dictionary.size());
                }
            }
        }
        dictionary.emplace_back(std::move(word));
    }
    // --------------------------------------------------------------------
    // initializing ncurses environment

    const std::size_t WINDOW_HEIGHT = 80;
    const std::size_t WINDOW_WIDTH = 43;


    initscr();
    noecho();
    cbreak();
    curs_set(0);


    WINDOW *window = newwin(WINDOW_HEIGHT, WINDOW_WIDTH, 0, 0);
    keypad(window, true);

    // --------------------------------------------------------------------
    // event loop

    std::string current_word;
    while (true) {
        mvwaddstr(window, 0, 0, ("Current word: " + (current_word.empty() ? "---" : current_word)).c_str());

        int pressed_key = wgetch(window);
        if (pressed_key == 27 /* KEY_ESCAPE */) break;
        wclear(window);
        wrefresh(window);


        if (pressed_key == KEY_BACKSPACE) {
            if (!current_word.empty()) current_word.pop_back();
        } else {
            current_word.push_back(char(pressed_key));
        }

        if (current_word.empty()) continue;

        auto init_letter_range = [](std::vector<std::size_t> &range) { for (std::size_t i = 0; i < ALPHABET_SIZE; i++) range.push_back(i); };
        std::vector<std::size_t> second_letter_range;
        if (current_word.size() <= 1) {
            init_letter_range(second_letter_range);
        } else {
            second_letter_range.push_back(current_word[1] - 'a');
        }

        std::vector<std::size_t> third_letter_range;
        if (current_word.size() <= 2) {
            init_letter_range(third_letter_range);
        } else {
            third_letter_range.push_back(current_word[2] - 'a');
        }

        std::size_t counter = 0;
        for (std::size_t i : second_letter_range) {
            for (std::size_t j : third_letter_range) {
                for (std::size_t id :
                     bundle[current_word[0] - 'a'][i][j]) {
                    if (dictionary[id].find(current_word) == std::string::npos) continue;
                    if (counter >= BOUND) break;
                    counter++;

                    mvwaddstr(window, counter, 0, (std::to_string(counter) + ". " + dictionary[id]).c_str());
                }
            }
        }
    }

    endwin();
}
