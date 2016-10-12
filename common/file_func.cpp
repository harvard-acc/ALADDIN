#include "file_func.h"

using namespace std;

void read_file(std::string file_name, std::vector<int>& output) {
  ifstream file;
#ifdef DDEBUG
  cerr << file_name << endl;
#endif
  file.open(file_name.c_str());
  std::string wholeline;
  if (file.is_open()) {
    while (!file.eof()) {
      getline(file, wholeline);
      if (wholeline.size() == 0)
        break;
      output.push_back(atoi(wholeline.c_str()));
    }
    file.close();
  } else {
    cerr << "file not open " << file_name << endl;
    exit(1);
  }
}

void read_gzip_string_file(std::string gzip_file_name,
                           unsigned size,
                           std::vector<std::string>& output) {
  gzFile gzip_file;
  gzip_file = gzopen(gzip_file_name.c_str(), "r");

#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  unsigned i = 0;
  while (!gzeof(gzip_file) && i < size) {
    char buffer[256];
    gzgets(gzip_file, buffer, 256);
    std::string s(buffer);
    output.at(i) = s.substr(0, s.size() - 1);
    i++;
  }
  gzclose(gzip_file);
  if (i == 0) {
    cerr << "file not open " << gzip_file_name << endl;
    exit(1);
  }
}

/*Read gz file into std::vector
Input: gzip-file-name, size of elements, std::vector to write to
*/
void read_gzip_file(std::string gzip_file_name,
                    unsigned size,
                    std::vector<int>& output) {
  gzFile gzip_file;
#ifdef DDEBUG
  std::cerr << gzip_file_name << std::endl;
#endif
  if (fileExists(gzip_file_name)) {
    gzip_file = gzopen(gzip_file_name.c_str(), "r");
    unsigned i = 0;
    while (!gzeof(gzip_file) && i < size) {
      char buffer[256];
      gzgets(gzip_file, buffer, 256);
      output.at(i) = strtol(buffer, NULL, 10);
      i++;
    }
    gzclose(gzip_file);
#ifdef DDEBUG
    std::cerr << "end of reading file" << gzip_file_name << std::endl;
#endif
  } else {
    cerr << "no such file " << gzip_file_name << endl;
  }
}

void read_gzip_unsigned_file(std::string gzip_file_name,
                             unsigned size,
                             std::vector<unsigned>& output) {
  gzFile gzip_file;
#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  gzip_file = gzopen(gzip_file_name.c_str(), "r");
  unsigned i = 0;
  while (!gzeof(gzip_file) && i < size) {
    char buffer[256];
    gzgets(gzip_file, buffer, 256);
    output.at(i) = (unsigned)strtol(buffer, NULL, 10);
    i++;
  }
  gzclose(gzip_file);
}

/*Read gz file into std::vector
Input: gzip-file-name, size of elements, std::vector to write to
*/
void read_gzip_file_no_size(std::string gzip_file_name,
                            std::vector<int>& output) {
  gzFile gzip_file;
#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  gzip_file = gzopen(gzip_file_name.c_str(), "r");
  unsigned i = 0;
  while (!gzeof(gzip_file)) {
    char buffer[256];
    gzgets(gzip_file, buffer, 256);
    int value;
    sscanf(buffer, "%d", &value);
    output.push_back(value);
    i++;
  }
  gzclose(gzip_file);

  if (i == 0) {
    cerr << "file not open " << gzip_file_name << endl;
    exit(1);
  }
}

/*Read gz with two element file into std::vector
Input: gzip-file-name, size of elements, std::vector to write to
*/
void read_gzip_2_unsigned_file(
    std::string gzip_file_name,
    unsigned size,
    std::vector<std::pair<unsigned, unsigned>>& output) {
  gzFile gzip_file;
#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  gzip_file = gzopen(gzip_file_name.c_str(), "r");
  unsigned i = 0;
  while (!gzeof(gzip_file) && i < size) {
    char buffer[256];
    gzgets(gzip_file, buffer, 256);
    unsigned element1, element2;
    sscanf(buffer, "%d,%d", &element1, &element2);
    output.at(i).first = element1;
    output.at(i).second = element2;
    i++;
  }
  gzclose(gzip_file);

  if (i == 0) {
    cerr << "file not open " << gzip_file_name << endl;
    exit(1);
  }
}

void read_gzip_1in2_unsigned_file(std::string gzip_file_name,
                                  unsigned size,
                                  std::vector<unsigned>& output) {
  gzFile gzip_file;
#ifdef DDEBUG
  cerr << gzip_file_name << "," << size << endl;
#endif
  gzip_file = gzopen(gzip_file_name.c_str(), "r");
  unsigned i = 0;
  while (!gzeof(gzip_file) && i < size) {
    char buffer[256];
    gzgets(gzip_file, buffer, 256);
    unsigned element1, element2;
    sscanf(buffer, "%d,%d", &element1, &element2);
    output.at(i) = element1;
    i++;
  }
  gzclose(gzip_file);
  if (i == 0) {
    cerr << "file not open " << gzip_file_name << endl;
    exit(1);
  }
}

void write_gzip_file(std::string gzip_file_name,
                     unsigned size,
                     std::vector<int>& output) {
  gzFile gzip_file;
#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  gzip_file = gzopen(gzip_file_name.c_str(), "w");
  for (unsigned i = 0; i < size; ++i)
    gzprintf(gzip_file, "%d\n", output.at(i));
  gzclose(gzip_file);
}

void write_gzip_bool_file(std::string gzip_file_name,
                          unsigned size,
                          std::vector<bool>& output) {
  gzFile gzip_file;
#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  gzip_file = gzopen(gzip_file_name.c_str(), "w");
  for (unsigned i = 0; i < size; ++i)
    gzprintf(gzip_file, "%d\n", output.at(i) ? 1 : 0);
  gzclose(gzip_file);
}

void write_gzip_unsigned_file(std::string gzip_file_name,
                              unsigned size,
                              std::vector<unsigned>& output) {
  gzFile gzip_file;
#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  gzip_file = gzopen(gzip_file_name.c_str(), "w");
  for (unsigned i = 0; i < size; ++i)
    gzprintf(gzip_file, "%u\n", output.at(i));
  gzclose(gzip_file);
}

void write_string_file(std::string file_name,
                       unsigned size,
                       std::vector<std::string>& output) {
  std::ofstream file;
  file.open(file_name.c_str());
  for (unsigned i = 0; i < size; ++i)
    file << output.at(i) << endl;
  file.close();
}

void write_gzip_string_file(std::string gzip_file_name,
                            unsigned size,
                            std::vector<std::string>& output) {
  gzFile gzip_file;
  gzip_file = gzopen(gzip_file_name.c_str(), "w");
#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  for (unsigned i = 0; i < size; ++i)
    gzprintf(gzip_file, "%s\n", output.at(i).c_str());
  gzclose(gzip_file);
}

bool fileExists(const std::string file_name) {
  struct stat buf;
  if (stat(file_name.c_str(), &buf) != -1)
    return true;
  return false;
}
