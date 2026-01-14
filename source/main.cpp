#include "CLI11.hpp"
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

const int8_t VERSION_MAJOR{1};
const int8_t VERSION_MINOR{5};
const int8_t RELEASE{1};

// Return codes
const int8_t SUCCESS{0};
const int8_t FILE_OPEN_FAILED{-1};

const int8_t UID_LENGTH{12};

const std::string CHARACTER_SET{"abcdefghijklmnopqrstuvwxyz"
                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "0123456789"};
const std::string SUPPORTED_FILE_EXTENSIONS[6]{".uid",  ".tres", ".tres",
                                               ".tscn", ".scn",  ".import"};

bool recursive{false};
bool verbose{false};

std::vector<std::filesystem::path> file_paths{};

/*
Iterates through each character in a string of UID_LENGTH size and places a
random character in its place.
*/
std::string generateRandomUID() {
  std::string result{};
  result.resize(UID_LENGTH);

  std::random_device random_device{};
  std::mt19937 random_number_generator(random_device());
  std::uniform_int_distribution<int> int_distribution(
      0, static_cast<int>(CHARACTER_SET.length() - 1));

  for (int i = 0; i < UID_LENGTH; i++) {
    result[i] = CHARACTER_SET[int_distribution(random_number_generator)];
  }

  return result;
}

// Prints unable to open file error message.
void printFileErrorMessage(const std::filesystem::path &file_path) {
  std::cout << "ERROR: Unable to open file: " << file_path.string()
            << "(Maybe invalid read/write permissions?)\n";
}

/*
Iterates through each line in a file and writes each line to a temporary
file, then removes the old file and renames temporary file to the
name of the old file. If a UID is found within a line, replaces it with a new
UID using generateRandomUID then writes the line.
*/
bool handleFile(const std::filesystem::path &file_path) {
  std::cout << "File: " << file_path.string() << '\n';
  std::ifstream input_file_stream(file_path);

  if (!input_file_stream.is_open()) {
    printFileErrorMessage(file_path);

    return false;
  }

  std::filesystem::path tempfile_path(file_path.string() + ".tmp");
  std::ofstream output_file_stream(tempfile_path);

  if (!output_file_stream.is_open()) {
    printFileErrorMessage(file_path);

    return false;
  }

  std::string line{};
  int line_count{};

  while (std::getline(input_file_stream, line)) {
    size_t uid_position{line.find("\"uid://")};

    if (uid_position == std::string::npos) {
      output_file_stream << line << '\n';

      continue;
    }

    // start of actual UID
    uid_position += 7;

    // end of actual UID
    size_t endquote_position{line.find('"', uid_position)};

    std::string new_uid{generateRandomUID()};

    if (verbose) {
      std::cout << "Replacing line: " << line << '\n';
      std::cout << "[UID: "
                << line.substr(uid_position, endquote_position - uid_position)
                << " | New UID: " << new_uid << "]\n";
    }

    // replace as many characters as the actual UID
    line.replace(uid_position, endquote_position - uid_position, new_uid);

    output_file_stream << line << '\n';
    line_count++;
  }

  std::cout << "Wrote " << line_count << " line(s).\n";

  input_file_stream.close();
  output_file_stream.close();

  std::remove(file_path.c_str());
  std::rename(tempfile_path.c_str(), file_path.c_str());

  return true;
}

/*
Checks if the file extension is valid.
*/
bool checkFileExtension(const std::filesystem::path &file_path) {
  for (std::string file_extension : SUPPORTED_FILE_EXTENSIONS) {
    if (file_path.extension().string() == file_extension) {
      return true;
    }
  }

  return false;
}

/*
Checks if entry is a file and calls checkFileExtension to validate its
extension. If both are true adds to file_paths vector.
*/
void handleDirectoryEntry(const std::filesystem::directory_entry &entry) {
  if (!entry.is_regular_file()) {
    return;
  }

  if (checkFileExtension(entry.path())) {
    file_paths.push_back(entry.path());
  }
}

void printRandomizingMessage(bool files = false) {
  if (files) {
    std::cout << "Randomizing UIDS of all godot file(s) listed";
  } else {
    std::cout << "Randomizing UIDS of all godot files in current directory";

    if (recursive) {
      std::cout << " and all subdirectories";
    }
  }

  std::cout << "...\n";
}

/*
Iterates through a directory and if recursive iteration is enabled, its
subdirectories.
*/
void randomizeDirectory() {
  printRandomizingMessage();

  if (recursive) {
    std::filesystem::recursive_directory_iterator directory_iterator(".");

    for (const std::filesystem::directory_entry &entry : directory_iterator) {
      handleDirectoryEntry(entry);
    }
  } else {
    std::filesystem::directory_iterator directory_iterator(".");

    for (const std::filesystem::directory_entry &entry : directory_iterator) {
      handleDirectoryEntry(entry);
    }
  }
}

/*
Iterates through each file in file_paths and calls handleFile for each one.
If directory iteration is enabled first calls randomizeDirectory.
*/
bool randomize(bool directory = true) {
  if (directory) {
    randomizeDirectory();
  } else {
    printRandomizingMessage(true);
  }

  for (const std::filesystem::path &file_path : file_paths) {
    if (!checkFileExtension(file_path)) {
      continue;
    }

    if (!handleFile(file_path)) {
      return false;
    }
  }

  return true;
}

int main(int argc, char **argv) {
  CLI::App app("Randomizes UIDs of godot resources.");

  app.add_option("-f, --file", file_paths, "Randomize the specified file(s)")
      ->check(CLI::ExistingFile);

  app.add_flag("-r, --recursive", recursive, "Recursively randomize");
  app.add_flag("-v, --verbose", verbose, "Verbosely randomize");

  argv = app.ensure_utf8(argv);
  CLI11_PARSE(app, argc, argv);

  std::cout << "godot-uid-fixer v" << VERSION_MAJOR << "." << VERSION_MINOR
            << "-" << RELEASE << "\n\n";

  if (!randomize(file_paths.empty())) {
    return FILE_OPEN_FAILED;
  }

  return SUCCESS;
}
