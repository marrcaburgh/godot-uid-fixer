#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <random>
#include <dirent.h>

const std::string CHARACTER_SET {
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
    };
const int STRING_LENGTH { 12 };

std::string generateRandomUID() {
    std::string result {};
    result.resize(STRING_LENGTH);

    std::random_device random_device {};
    std::mt19937 random_number_generator { random_device() };
    std::uniform_int_distribution<int> int_distribution { 0, static_cast<int>(CHARACTER_SET.length() - 1) };

    for (int i = 0; i < STRING_LENGTH; i++) {
        result[i] = CHARACTER_SET[int_distribution(random_number_generator)];
    }

    return result;
}

bool handleFile(struct dirent* ent) {
    std::filesystem::path file_path { ent->d_name };

    if (file_path.extension() != ".tres") {
        return true;
    }

    std::ifstream input_file_stream { file_path };

    if (!input_file_stream.is_open()) {
        std::cerr << "ERROR: Unable to open file: " << file_path << " (Maybe invalid read/write permissions?)\n";
        return false;
    }
    
    std::filesystem::path tempfile_path { file_path.string() + ".tmp" };
    std::ofstream output_file_stream { tempfile_path };

    if (!output_file_stream.is_open()) {
        std::cerr << "ERROR: Unable to open temp file: " << tempfile_path << " (Maybe invalid read/write permissions?)\n";
        return false;
    }

    std::cout << "Reading file: " << file_path << '\n';
    std::string line {};

    while (std::getline(input_file_stream, line)) {
        size_t uid_position { line.find("\"uid://") };

        if (uid_position == std::string::npos) {
            output_file_stream << line << '\n';
            continue;
        }

        uid_position += 7;
        
        std::cout << "line: " << line << '\n';
        size_t endquote_pos { line.find('"', uid_position) };
        line.replace(uid_position, endquote_pos - uid_position, generateRandomUID());
        std::cout << "new line: " << line << '\n';

        output_file_stream << line << '\n';
    }

    input_file_stream.close();
    output_file_stream.close();
    std::remove(file_path.c_str());
    std::rename(tempfile_path.c_str(), file_path.c_str());

    return true;
}

int main() {
    DIR* directory { opendir(".") };

    if (!directory) {
        std::cerr << "ERROR: Unable to open current directory! (Maybe invalid read/write permissions?)\n";
        return -1;
    }

    std::cout << "Randomizing UIDs of all godot resources in current directory...\n";

    struct dirent* ent {};

    while ((ent = readdir(directory)) != NULL) {
        if (!handleFile(ent)) {
            return -2;
        }
    }

    std::cout << "Done!\n";

    closedir(directory);

    return 0;
}
