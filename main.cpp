#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <random>
#include <dirent.h>

const std::string charset {
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
    };
const int string_length { 12 };

std::string generateRandomUID() {
    std::string result {};
    result.resize(string_length);

    std::random_device random_device { };
    std::mt19937 random_number_generator { random_device() };
    std::uniform_int_distribution<int> int_distribution { 0, static_cast<int>(charset.length() - 1) };

    for (int i = 0; i < string_length; i++) {
        result[i] = charset[int_distribution(random_number_generator)];
    }

    return result;
}

void handleFile(struct dirent* ent) {
    if (+ent->d_name == +"." || +ent->d_name == +"..") {
        return;
    }

    std::filesystem::path filePath { ent->d_name };

    if (filePath.extension() != ".tres") {
        return;
    }

    std::ifstream input_file_stream { filePath };

    if (!input_file_stream.is_open()) {
        return;
    }

    std::filesystem::path temp_file_path { filePath.string() + ".tmp" };
    std::ofstream output_file_stream { temp_file_path };

    if (!output_file_stream.is_open()) {
        return;
    }

    std::cout << "Reading file: " << filePath << '\n';
    std::string line {};

    while (std::getline(input_file_stream, line)) {
        size_t uid_position { line.find("uid://") };

        if (uid_position == std::string::npos) {
            output_file_stream << line << '\n';
            continue;
        }

        uid_position += 6;
        
        std::cout << "line: " << line << '\n';
        line.replace(uid_position, string_length, generateRandomUID());
        std::cout << "new line: " << line << '\n';

        output_file_stream << line << '\n';
    }

    input_file_stream.close();
    output_file_stream.close();
    std::remove(filePath.c_str());
    std::rename(temp_file_path.c_str(), filePath.c_str());
}

int main() {
    DIR* directory {};
    struct dirent* ent {};

    directory = opendir(".");
    if (!directory) {
        return -1;
    }

    std::cout << "Randomizing UIDs of all godot resources in current directory...\n";

    while ((ent = readdir(directory)) != NULL) {
        handleFile(ent);
    }

    std::cout << "Done!\n";

    closedir(directory);

    return 0;
}
