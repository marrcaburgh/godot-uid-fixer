#include "CLI11.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <random>
#include <dirent.h>

const double VERSION = 1.3;
const std::string CHARACTER_SET {
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
    };
const int STRING_LENGTH { 12 };

bool verbose { false };
bool recursive { false };

std::string generateRandomUID() {
    std::string result {};
    result.resize(STRING_LENGTH);

    std::random_device random_device {};
    std::mt19937 random_number_generator(random_device());
    std::uniform_int_distribution<int> int_distribution (0, static_cast<int>(CHARACTER_SET.length() - 1));

    for (int i = 0; i < STRING_LENGTH; i++) {
        result[i] = CHARACTER_SET[int_distribution(random_number_generator)];
    }

    return result;
}

bool handleFile(std::filesystem::path file_path) {
    if (file_path.extension() != ".tres") {
        return true;
    }

    std::cout << "File: " << file_path.filename() << '\n';
    std::cout << "Path: " << file_path.parent_path() << '\n';

    std::ifstream input_file_stream(file_path);

    if (!input_file_stream.is_open()) {
        std::cerr << "ERROR: Unable to open file: " << file_path << " (Maybe invalid read/write permissions?)\n";
        return false;
    }
    
    std::filesystem::path tempfile_path(file_path.string() + ".tmp");
    std::ofstream output_file_stream(tempfile_path);

    if (!output_file_stream.is_open()) {
        std::cerr << "ERROR: Unable to open temp file: " << tempfile_path << " (Maybe invalid read/write permissions?)\n";
        return false;
    }

    std::string line {};
    int line_count {};

    while (std::getline(input_file_stream, line)) {
        size_t uid_position { line.find("\"uid://") };

        if (uid_position == std::string::npos) {
            output_file_stream << line << '\n';
            continue;
        }

        uid_position += 7;
        size_t endquote_position { line.find('"', uid_position) };

        if (verbose) {
            std::cout << "Replacing line: " << line << '\n';
            std::cout << "UID: " << line.substr(uid_position, endquote_position - uid_position) << '\n';
        }
        
        line.replace(uid_position, endquote_position - uid_position, generateRandomUID());

        if (verbose) {
            std::cout << "Replaced line: " << line << '\n';
            std::cout << "UID: " << line.substr(uid_position, endquote_position - uid_position) << '\n';
        }

        output_file_stream << line << '\n';
        line_count += 1;
    }

    std::cout << "Wrote " << line_count << " line(s).\n";

    input_file_stream.close();
    output_file_stream.close();
    std::remove(file_path.c_str());
    std::rename(tempfile_path.c_str(), file_path.c_str());

    return true;
}



int main(int argc, char **argv) {
    CLI::App app("Randomizes UIDs of all godot resources in the current directory.");

    app.add_flag("-v, --verbose", verbose, "verbosely randomize uids");
    app.add_flag("-r, --recursive", recursive, "recursively randomize uids");

    argv = app.ensure_utf8(argv);
    CLI11_PARSE(app, argc, argv);

    std::cout << "godot-uid-fixer v" << VERSION << "\n\n";

    std::filesystem::path directory_path(".");

    if (recursive) {
        std::cout << "Randomizing UIDs of all godot resources in current directory and all subdirectories...\n";
        std::filesystem::recursive_directory_iterator directory_iterator(directory_path);
        for (const std::filesystem::directory_entry& entry : directory_iterator) {
            if (entry.is_regular_file()) {
                if (!handleFile(entry.path())) {
                  return -2;
                }
            }
        }
    } else {
        std::cout << "Randomizing UIDs of all godot resources in current directory...\n";
        std::filesystem::directory_iterator directory_iterator(directory_path);
        for (const std::filesystem::directory_entry& entry : directory_iterator) {
            if (entry.is_regular_file()) {
                if (!handleFile(entry.path())) {
                    return -2;
                }
            }
        }
    }

    std::cout << "Done!\n";

    return 0;
}
