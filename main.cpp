#include "ncurses.h"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>


// --------------------------------------------------------------------
// Ncurses constants

const std::size_t WINDOW_HEIGHT = 80;
const std::size_t WINDOW_WIDTH = 43;

const std::size_t PROGRAM_TIME_LINE = 0;
const std::size_t REQUEST_TIME_LINE = PROGRAM_TIME_LINE + 1;
const std::size_t CURRENT_WORD_LINE = REQUEST_TIME_LINE + 1;
const std::size_t MATCHES_LINE = CURRENT_WORD_LINE + 1;
// --------------------------------------------------------------------


void dictionary_init(const std::string &file_path, std::vector<std::string> &dictionary) {
    std::ifstream file(file_path, std::ifstream::in);
    if (!file.good()) throw std::runtime_error("Couldn't open file: " + file_path);

    for (std::string word; file >> word;)
        dictionary.push_back(std::move(word));
}

struct SubstringSearch {
    std::unordered_map<std::size_t, std::unordered_map<std::size_t, std::unordered_map<std::size_t, std::vector<std::size_t>>>> bundle;
    std::vector<std::size_t> matches;

    explicit SubstringSearch(const std::vector<std::string> &dictionary) {
        for (std::size_t i = 0; i < dictionary.size(); i++) {
            const std::string &word = dictionary[i];
            for (std::size_t j = 0; j + 1 < word.size(); j++) {
                if (j + 2 < word.size()) {
                    bundle[word[j] - 'A'][word[j + 1] - 'A'][word[j + 2] - 'A'].push_back(i);
                } else {
                    bundle[word[j] - 'A'][word[j + 1] - 'A'][0].push_back(i);
                }
            }
        }
    }

    void search(const std::string &current_word, const std::vector<std::string> &dictionary) {
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
            for (std::size_t j : third_letter_range) {
                for (std::size_t id : bundle[current_word[0] - 'A'][i][j]) {
                    if (dictionary[id].find(current_word) == std::string::npos) continue;
                    matches.push_back(id);
                }
            }
        }
    }
};

void print(WINDOW *&window, const std::vector<std::size_t> &matches, const std::vector<std::string> &dictionary, std::size_t from) {
    for (std::size_t i = from; i < std::min(matches.size(), from + WINDOW_HEIGHT); i++)
        mvwaddstr(window, MATCHES_LINE + i - from, 0, (std::to_string(i) + ". " + dictionary[matches[i]]).c_str());
}


// --------------------------------------------------------------------
// This is for time measurement

int calculate_time(const std::chrono::time_point<std::chrono::high_resolution_clock> &start) {
    return static_cast<int>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count() / 1e4);
}
// --------------------------------------------------------------------


int main() {
    auto program_start_time = std::chrono::high_resolution_clock::now();
    // --------------------------------------------------------------------
    // Extracting command line arguments

    std::string FILE_PATH = "words.txt";
    // --------------------------------------------------------------------
    // Extracting words from dictionary

    std::vector<std::string> dictionary;
    dictionary_init(FILE_PATH, dictionary);
    // --------------------------------------------------------------------
    // Initializing searches

    SubstringSearch substring_search(dictionary);

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

    int init_duration = calculate_time(program_start_time);
    std::string current_word;
    std::size_t show_range = 0;
    int request_duration = 0;
    while (true) {
        mvwaddstr(window, PROGRAM_TIME_LINE, 0, ("Dictionary initialized in: 0." + std::to_string(init_duration) + "s").c_str());
        mvwaddstr(window, REQUEST_TIME_LINE, 0, ("Request done in: 0." + std::to_string(request_duration) + "s").c_str());
        mvwaddstr(window, CURRENT_WORD_LINE, 0, ("Current word > " + (current_word.empty() ? "[Press any letter]" : current_word)).c_str());

        int pressed_key = wgetch(window);
        if (pressed_key == 27 /* KEY_ESCAPE */) break;
        wclear(window);
        wrefresh(window);


        if (pressed_key == KEY_BACKSPACE) {
            if (!current_word.empty()) current_word.pop_back();
            if (current_word.empty()) continue;
            show_range = 0;
            auto start = std::chrono::high_resolution_clock::now();
            substring_search.search(current_word, dictionary);
            request_duration = calculate_time(start);
        } else if (pressed_key == KEY_UP) {
            if (show_range != 0) show_range--;
        } else if (pressed_key == KEY_DOWN) {
            show_range++;
        } else {
            current_word.push_back(char(pressed_key));
            show_range = 0;
            auto start = std::chrono::high_resolution_clock::now();
            substring_search.search(current_word, dictionary);
            request_duration = calculate_time(start);
        }
        print(window, substring_search.matches, dictionary, show_range);
    }

    endwin();
}
