#include <iostream>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <vector>
#include "file_tools.hpp"
using namespace std;

namespace fs = std::filesystem;

File::File()
{
    width = -1;
    height = -1;
    size = -1;
    color = File::file_color::UNSIGNED;
    filename = "";
    fullpath = "";
}

File::File(int w, int h, file_color c, string path)
{
    width = w;
    height = h;
    size = -1;
    color = c;
    this->set_filename();
    this->set_fullpath(path);
}

File::File(int w, int h, file_color c, string path, int file_number)
{
    width = w;
    height = h;
    size = -1;
    color = c;
    this->set_filename_numbered(file_number);
    this->set_fullpath(path);
}

// File getters
int File::get_height()
{
    return height;
}

int File::get_width()
{
    return width;
}

int File::get_size()
{
    return size;
}

string File::get_filename()
{
    return filename;
}

string File::get_fullpath()
{
    return fullpath;
}

File::file_color File::get_color()
{
    return color;
}

// File setters
void File::set_size(int s)
{
    size = s;
}

void File::set_filename()
{
    filename = create_filename(color, width, height);
}

void File::set_filename_numbered(int file_number)
{
    filename = create_filename(color, width, height, file_number);
}

void File::set_fullpath(string path)
{
    fullpath = path + filename;
}

// Aux methods
string color_to_string(File::file_color c)
{
    switch (c)
    {
    case File::RED:
        return "RED";
    case File::GREEN:
        return "GREEN";
    case File::BLUE:
        return "BLUE";
    case File::UNSIGNED:
        return "UNSIGNED";
    default:
        return "UNKNOWN";
    }
}

uint8_t color_to_byte(File::file_color c)
{
    switch (c)
    {
    case File::RED:
        return 0;
    case File::GREEN:
        return 1;
    case File::BLUE:
        return 2;
    case File::UNSIGNED:
        return 3;
    default:
        return 4;
    }
}

File::file_color byte_to_color(std::uint8_t &byte)
{
    switch (byte)
    {
    case 0:
        return File::file_color::RED;
    case 1:
        return File::file_color::GREEN;
    case 2:
        return File::file_color::BLUE;
    case 3:
        return File::file_color::UNSIGNED;
    default:
        return File::file_color::UNSIGNED;
    }
}

string create_filename(File::file_color color, int width, int height)
{
    return color_to_string(color) + "_" + to_string(width) + "x" + to_string(height) + ".bin";
}

string create_filename(string color, int width, int height)
{
    return color + "_" + to_string(width) + "x" + to_string(height) + ".bin";
}

string create_filename(File::file_color color, int width, int height, int file_number)
{
    return color_to_string(color) + "_" + to_string(width) + "x" + to_string(height) + "_" + to_string(file_number) + ".bin";
}

// path methods

void ensure_directory_exists(const std::string &path)
{
    // Remove last character "/"
    std::string dir_path = path.substr(0, path.length() - 1);

    // Check if directory exists, if not create it
    if (!fs::exists(dir_path))
    {
        fs::create_directories(dir_path);
    }
}

string get_files_directory(string files_relative_directory)
{ // linux
    vector<char> path_buffer(4096, 0);

    ssize_t count = readlink("/proc/self/exe", path_buffer.data(), 4096);

    if (count == -1)
        throw std::runtime_error("readlink failed");
    path_buffer[count] = '\0';

    fs::path fullpath(path_buffer.data());
    fs::path parent_path = fullpath.parent_path().parent_path();
    string path = parent_path.string() + "/" + files_relative_directory + "/"; // client/files for client; server/files for server

    ensure_directory_exists(path);

    return path;
}

// file methods

void colored_file_creator(File &file)
{
    int height = file.get_height();
    int width = file.get_width();

    ofstream outputFile(file.get_fullpath(), std::ios::trunc | std::ios::binary);

    vector<std::byte> color_code; // define color code between rgb values

    if (file.get_color() == File::UNSIGNED)
    {
        throw std::runtime_error(file.get_filename() + " didn't get color assigned!");
    }
    else if (file.get_color() == File::RED)
    {
        color_code.push_back(byte{255});
        color_code.push_back(byte{0});
        color_code.push_back(byte{0});
    }
    else if (file.get_color() == File::GREEN)
    {
        color_code.push_back(byte{0});
        color_code.push_back(byte{255});
        color_code.push_back(byte{0});
    }
    else
    {
        color_code.push_back(byte{0});
        color_code.push_back(byte{0});
        color_code.push_back(byte{255});
    }

    if (outputFile.is_open())
    {
        int size = 0;

        int color_size;

        for (int h = 0; h < height; h++)
        {
            for (int l = 0; l < width; l++)
            {
                color_size = color_code.size();
                size += color_size;

                outputFile.write(reinterpret_cast<const char *>(color_code.data()), color_size);
            }
        }
        file.set_size(size);

        outputFile.close();
    }
    else
    {
        throw std::runtime_error("file not created successfully");
    }
}

std::array<File, 3> files_creator(int width, int height, string files_directory)
{

    // get path for file output (files folder)
    string path = get_files_directory(files_directory);

    // create three file objects (red, green, blue)
    File file_red = File(width, height, File::file_color::RED, path);
    File file_green = File(width, height, File::file_color::GREEN, path);
    File file_blue = File(width, height, File::file_color::BLUE, path);

    // write them to file
    colored_file_creator(file_red);
    colored_file_creator(file_green);
    colored_file_creator(file_blue);

    std::array<File, 3> files = {file_red, file_blue, file_green};

    return files;
}

bool File::check_file_size(File &file)
{
    ifstream file_stream(file.get_fullpath(), std::ios::binary | std::ios::ate);
    if (!file_stream.is_open())
    {
        cout << "Couldn't check file at: " << file.get_fullpath() << endl;
        return 0;
    }
    streampos size = file_stream.tellg();
    file_stream.close();
    long long expected_size = 3 * file.get_width() * file.get_height();
    if (static_cast<long long>(size) == expected_size)
        return 1;

    return 0;
}