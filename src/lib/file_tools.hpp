#ifndef FILE_TOOLS_HPP
#define FILE_TOOLS_HPP

#include <string>
#include <filesystem>

class File
{
public:
    enum file_color
    {
        RED,
        GREEN,
        BLUE,
        UNSIGNED
    };

private:
    std::string filename;
    std::string fullpath; // path + filename
    int height;
    int width;
    int size;
    file_color color;

public:
    File();
    File(int w, int h, file_color c, std::string path, int file_number);
    File(int width, int height, file_color color, std::string path);
    int get_height();
    int get_width();
    int get_size();
    file_color get_color();
    std::string get_filename();
    std::string get_fullpath();
    void set_size(int size);
    void set_filename();
    void set_filename_numbered(int file_number);
    void set_fullpath(std::string path);

    static bool check_file_size(File &file);
};

std::string color_to_string(File::file_color c);

uint8_t color_to_byte(File::file_color c);

File::file_color byte_to_color(std::uint8_t &byte);

std::string create_filename(std::string color, int width, int height);

std::string create_filename(File::file_color color, int width, int height);

std::string create_filename(File::file_color color, int width, int height, int file_number);

void colored_file_creator(File &file);

std::array<File, 3> files_creator(int width, int height, std::string files_directory);

std::string get_files_directory(std::string files_directory);

void ensure_directory_exists(const std::string &path);

#endif
