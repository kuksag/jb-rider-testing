#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "ncurses.h"

// --------------------------------------------------------------------
// Ncurses constants

const std::size_t WINDOW_HEIGHT = 80;
const std::size_t WINDOW_WIDTH = 43;

const std::size_t CURRENT_WORD_LINE = 0;
const std::size_t MATCHES_LINE = CURRENT_WORD_LINE + 1;
// --------------------------------------------------------------------

void dictionary_init(
    std::unordered_map<
        std::size_t,
        std::unordered_map<
            std::size_t,
            std::unordered_map<std::size_t, std::vector<std::size_t>>>> &bundle,
    std::vector<std::string> &dictionary, const std::string &file_path) {
    std::ifstream file(file_path, std::ifstream::in);
    if (!file.good())
        throw std::runtime_error("Couldn't open file: " + file_path);

    for (std::string word; file >> word;) dictionary.push_back(std::move(word));

    for (std::size_t i = 0; i < dictionary.size(); i++) {
        const std::string &word = dictionary[i];
        for (std::size_t j = 0; j + 1 < word.size(); j++) {
            auto unique_push_back = [](std::vector<std::size_t> &data,
                                       std::size_t value) {
                if (data.empty() || data.back() != value) data.push_back(value);
            };
            if (j + 2 < word.size()) {
                unique_push_back(
                    bundle[word[j] - 'A'][word[j + 1] - 'A'][word[j + 2] - 'A'],
                    i);
            } else {
                unique_push_back(bundle[word[j] - 'A'][word[j + 1] - 'A'][0],
                                 i);
            }
        }
    }
}

void search(
    std::unordered_map<
        std::size_t,
        std::unordered_map<
            std::size_t,
            std::unordered_map<std::size_t, std::vector<std::size_t>>>> &bundle,
    std::vector<std::size_t> &matches, std::vector<std::string> &dictionary,
    const std::string &current_word) {
    if (current_word.empty()) return;
    auto init_letter_range = [](std::vector<std::size_t> &range) {
        for (std::size_t i = 0; i < (1 << 8); i++) range.push_back(i);
    };
    std::vector<std::size_t> second_letter_range;

    if (current_word.size() <= 1) {
        init_letter_range(second_letter_range);
    } else {
        second_letter_range.push_back(current_word[1] - 'A');
    }

    std::vector<std::size_t> third_letter_range;
    if (current_word.size() <= 2) {
        init_letter_range(third_letter_range);
    } else {
        third_letter_range.push_back(current_word[2] - 'A');
    }

    matches.clear();
    for (std::size_t i : second_letter_range) {
        if (matches.size() > WINDOW_HEIGHT) break;
        for (std::size_t j : third_letter_range) {
            if (matches.size() > WINDOW_HEIGHT) break;
            for (std::size_t id : bundle[current_word[0] - 'A'][i][j]) {
                if (matches.size() > WINDOW_HEIGHT) break;
                if (dictionary[id].find(current_word) == std::string::npos)
                    continue;
                matches.push_back(id);
            }
        }
    }
}

void print(WINDOW *&window, std::vector<std::string> &dictionary,
           std::vector<std::size_t> &matches, std::size_t show_range,
           const std::string &current_word) {
    if (current_word.empty()) return;
    for (std::size_t i = show_range;
         i < std::min(matches.size(), show_range + WINDOW_HEIGHT); i++) {
        std::string line = std::to_string(i) + ". " + dictionary[matches[i]];
        std::size_t pos = line.find(current_word);
//        if (pos == std::string::npos) continue;
        for (std::size_t j = 0; j < line.size(); j++) {
            if (j == pos) wattron(window, A_BOLD);
            mvwaddch(window, MATCHES_LINE + i - show_range, j, line[j]);
            if (j + 1 == pos + current_word.size()) wattroff(window, A_BOLD);
        }
    }
}

int main(int argc, char *argv[]) {
    // --------------------------------------------------------------------
    std::unordered_map<
        std::size_t,
        std::unordered_map<
            std::size_t,
            std::unordered_map<std::size_t, std::vector<std::size_t>>>>
        bundle;
    std::vector<std::size_t> matches;
    std::vector<std::string> dictionary;
    // --------------------------------------------------------------------
    // Extracting command line arguments

    std::string FILE_PATH = "words.txt";
    if (argc == 2) {
        FILE_PATH = argv[1];
    } else if (argc > 2) {
        throw std::runtime_error("Expected zero or one argument, got: " +
                                 std::to_string(argc - 1));
    }
    // --------------------------------------------------------------------
    // Extracting words from dictionary

    std::thread thread1(dictionary_init, std::ref(bundle), std::ref(dictionary),
                        FILE_PATH);
    // --------------------------------------------------------------------
    // Initializing ncurses environment

    initscr();
    noecho();
    cbreak();
    curs_set(0);

    WINDOW *window = newwin(WINDOW_HEIGHT, WINDOW_WIDTH, 0, 0);
    keypad(window, true);
    // --------------------------------------------------------------------
    // Event loop

    std::string current_word;
    std::size_t show_range = 0;
    while (true) {
        mvwaddstr(window, CURRENT_WORD_LINE, 0,
                  ("Current word > " +
                   (current_word.empty() ? "[Press any letter]" : current_word))
                      .c_str());

        int pressed_key = wgetch(window);
        if (pressed_key == 27 /* KEY_ESCAPE */) break;
        wclear(window);
        wrefresh(window);

        if (pressed_key == KEY_UP) {
            if (show_range != 0) show_range--;
        } else if (pressed_key == KEY_DOWN) {
            show_range++;
        } else {
            show_range = 0;
            if (pressed_key == KEY_BACKSPACE) {
                if (!current_word.empty()) current_word.pop_back();
            } else {
                current_word.push_back(static_cast<char>(pressed_key));
            }
        }

        search(bundle, matches, dictionary, current_word);
        print(window, dictionary, matches, show_range, current_word);
    }

    thread1.join();
    endwin();
}
