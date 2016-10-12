#ifndef FILE_FUNC_H
#define FILE_FUNC_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <assert.h>
#include <sys/stat.h>
#include <zlib.h>

void write_gzip_file(std::string gzip_file_name,
                     unsigned size,
                     std::vector<int>& output);
void write_gzip_bool_file(std::string gzip_file_name,
                          unsigned size,
                          std::vector<bool>& output);
void write_gzip_unsigned_file(std::string gzip_file_name,
                              unsigned size,
                              std::vector<unsigned>& output);
void write_gzip_string_file(std::string gzip_file_name,
                            unsigned size,
                            std::vector<std::string>& output);
void write_string_file(std::string file_name,
                       unsigned size,
                       std::vector<std::string>& output);

void read_file(std::string file_name, std::vector<int>& output);
void read_gzip_file(std::string gzip_file_name,
                    unsigned size,
                    std::vector<int>& output);
void read_gzip_unsigned_file(std::string gzip_file_name,
                             unsigned size,
                             std::vector<unsigned>& output);
void read_gzip_string_file(std::string gzip_file_name,
                           unsigned size,
                           std::vector<std::string>& output);
void read_gzip_file_no_size(std::string gzip_file_name,
                            std::vector<int>& output);
void read_gzip_2_unsigned_file(
    std::string gzip_file_name,
    unsigned size,
    std::vector<std::pair<unsigned, unsigned>>& output);
void read_gzip_1in2_unsigned_file(std::string gzip_file_name,
                                  unsigned size,
                                  std::vector<unsigned>& output);

bool fileExists(const std::string file_name);

#endif
