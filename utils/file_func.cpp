#include "file_func.h"

void read_file(string file_name, vector<int> &output)
{
  ifstream file;
#ifdef DDEBUG
  cerr << file_name << endl;
#endif
  file.open(file_name.c_str());
  string wholeline;
  if (file.is_open())
  {
    while (!file.eof())
    {
      getline(file, wholeline);
      if (wholeline.size() == 0)
        break;
      output.push_back(atoi(wholeline.c_str()));
    }
    file.close();
  }
  else
  {
    cerr << "file not open " << file_name << endl;
    exit(0);
  }
}

void read_gzip_string_file ( string gzip_file_name, unsigned size,
  vector<string> &output)
{
  gzFile gzip_file;
  gzip_file = gzopen(gzip_file_name.c_str(), "r");

#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  unsigned i = 0;
  while(!gzeof(gzip_file) && i< size)
  {
    char buffer[256];
    gzgets(gzip_file, buffer, 256);
    string s(buffer);
    output.at(i) = s.substr(0,s.size()-1);
    i++;
  }
  gzclose(gzip_file);
  if (i == 0)
  {
    cerr << "file not open " << gzip_file_name << endl;
    exit(0);
  }
}

/*Read gz file into vector
Input: gzip-file-name, size of elements, vector to write to
*/
void read_gzip_file(string gzip_file_name, unsigned size, vector<int> &output)
{
  gzFile gzip_file;
#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  if (fileExists(gzip_file_name))
  {
    gzip_file = gzopen(gzip_file_name.c_str(), "r");
    unsigned i = 0;
    while(!gzeof(gzip_file) && i< size)
    {
      char buffer[256];
      gzgets(gzip_file, buffer, 256);
      output.at(i) = strtol(buffer, NULL, 10);
      i++;
    }
    gzclose(gzip_file);
    
#ifdef DDEBUG
    cerr << "end of reading file" << gzip_file_name << endl;
#endif
  }
  else
  {
    cerr << "no such file " <<  gzip_file_name << endl;
  }
}
void read_gzip_unsigned_file(string gzip_file_name, unsigned size,
  vector<unsigned> &output)
{
  gzFile gzip_file;
#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  gzip_file = gzopen(gzip_file_name.c_str(), "r");
  unsigned i = 0;
  while(!gzeof(gzip_file) && i< size)
  {
    char buffer[256];
    gzgets(gzip_file, buffer, 256);
    output.at(i) = (unsigned)strtol(buffer, NULL, 10);
    i++;
  }
  gzclose(gzip_file);
}

/*Read gz file into vector
Input: gzip-file-name, size of elements, vector to write to
*/
void read_gzip_file_no_size(string gzip_file_name, vector<int> &output)
{
  gzFile gzip_file;
#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  gzip_file = gzopen(gzip_file_name.c_str(), "r");
  unsigned i = 0;
  while(!gzeof(gzip_file) )
  {
    char buffer[256];
    gzgets(gzip_file, buffer, 256);
    int value;
    sscanf(buffer, "%d", &value);
    output.push_back(value);
    i++;
  }
  gzclose(gzip_file);
  
  if (i == 0)
  {
    cerr << "file not open " << gzip_file_name << endl;
    exit(0);
  }
}

/*Read gz with two element file into vector
Input: gzip-file-name, size of elements, vector to write to
*/
void read_gzip_2_unsigned_file(string gzip_file_name, unsigned size,
  vector< pair<unsigned, unsigned> > &output)
{
  gzFile gzip_file;
#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  gzip_file = gzopen(gzip_file_name.c_str(), "r");
  unsigned i = 0;
  while(!gzeof(gzip_file) && i< size)
  {
    char buffer[256];
    gzgets(gzip_file, buffer, 256);
    unsigned element1, element2;
    sscanf(buffer, "%d,%d", &element1, &element2);
    output.at(i).first = element1;
    output.at(i).second = element2;
    i++;
  }
  gzclose(gzip_file);
  
  if (i == 0)
  {
    cerr << "file not open " << gzip_file_name << endl;
    exit(0);
  }
}

void read_gzip_1in2_unsigned_file(string gzip_file_name, unsigned size,
  vector<unsigned> &output)
{
  gzFile gzip_file;
#ifdef DDEBUG
  cerr << gzip_file_name << "," << size << endl;
#endif
  gzip_file = gzopen(gzip_file_name.c_str(), "r");
  unsigned i = 0;
  while(!gzeof(gzip_file) && i< size)
  {
    char buffer[256];
    gzgets(gzip_file, buffer, 256);
    unsigned element1, element2;
    sscanf(buffer, "%d,%d", &element1, &element2);
    output.at(i) = element1;
    i++;
  }
  gzclose(gzip_file);
  if (i == 0)
  {
    cerr << "file not open " << gzip_file_name << endl;
    exit(0);
  }
}

void write_gzip_file(string gzip_file_name, unsigned size, vector<int> &output)
{
  gzFile gzip_file;
#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  gzip_file = gzopen(gzip_file_name.c_str(), "w");
  for (unsigned i = 0; i < size; ++i)
    gzprintf(gzip_file, "%d\n", output.at(i));
  gzclose(gzip_file);
}

void write_gzip_bool_file(string gzip_file_name, unsigned size, vector<bool> &output)
{
  gzFile gzip_file;
#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  gzip_file = gzopen(gzip_file_name.c_str(), "w");
  for (unsigned i = 0; i < size; ++i)
    gzprintf(gzip_file, "%d\n", output.at(i));
  gzclose(gzip_file);
}

void write_gzip_unsigned_file(string gzip_file_name, unsigned size,
vector<unsigned> &output)
{
  gzFile gzip_file;
#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  gzip_file = gzopen(gzip_file_name.c_str(), "w");
  for (unsigned i = 0; i < size; ++i)
    gzprintf(gzip_file, "%u\n", output.at(i));
  gzclose(gzip_file);
}

void write_string_file(string file_name, unsigned size, vector<string> &output)
{
  ofstream file;
  file.open(file_name.c_str());
  for (unsigned i = 0; i < size; ++i)
    file << output.at(i) << endl;
  file.close();
}

void write_gzip_string_file(string gzip_file_name, unsigned size, vector<string> &output)
{
  gzFile gzip_file;
  gzip_file = gzopen(gzip_file_name.c_str(), "w");
#ifdef DDEBUG
  cerr << gzip_file_name << endl;
#endif
  for (unsigned i = 0; i < size; ++i)
    gzprintf(gzip_file, "%s\n", output.at(i).c_str());
  gzclose(gzip_file);
}

void parse_config(std::string bench, std::string config_file_name)
{
  ifstream config_file;
  config_file.open(config_file_name);
  std::string wholeline;

  std::vector<std::string> flatten_config;
  std::vector<std::string> unrolling_config;
  std::vector<std::string> partition_config;
  std::vector<std::string> comp_partition_config;
  std::vector<std::string> pipelining_config;

  while(!config_file.eof())
  {
    wholeline.clear();
    getline(config_file, wholeline);
    if (wholeline.size() == 0)
      break;
    string type, rest_line;
    int pos_end_tag = wholeline.find(",");
    if (pos_end_tag == -1)
      break;
    type = wholeline.substr(0, pos_end_tag);
    rest_line = wholeline.substr(pos_end_tag + 1);
    if (!type.compare("flatten"))
      flatten_config.push_back(rest_line); 

    else if (!type.compare("unrolling"))
      unrolling_config.push_back(rest_line); 

    else if (!type.compare("partition"))
      if (wholeline.find("complete") == std::string::npos)
        partition_config.push_back(rest_line); 
      else 
        comp_partition_config.push_back(rest_line); 
    else if (!type.compare("pipelining"))
      pipelining_config.push_back(rest_line);
    else
    {
      cerr << "what else? " << wholeline << endl;
      exit(0);
    }
  }
  config_file.close();
  if (flatten_config.size() != 0)
  {
    string file_name(bench);
    file_name += "_flatten_config";
    ofstream output;
    output.open(file_name);
    for (unsigned i = 0; i < flatten_config.size(); ++i)
      output << flatten_config.at(i) << endl;
    output.close();
  }
  if (unrolling_config.size() != 0)
  {
    string file_name(bench);
    file_name += "_unrolling_config";
    ofstream output;
    output.open(file_name);
    for (unsigned i = 0; i < unrolling_config.size(); ++i)
      output << unrolling_config.at(i) << endl;
    output.close();
  }
  if (pipelining_config.size() != 0)
  {
    string pipelining(bench);
    pipelining += "_pipelining_config";

    ofstream pipe_config;
    pipe_config.open(pipelining);
    for (unsigned i = 0; i < pipelining_config.size(); ++i)
      pipe_config << pipelining_config.at(i) << endl;
    pipe_config.close();
  }
  if (partition_config.size() != 0)
  {
    string partition(bench);
    partition += "_partition_config";

    ofstream part_config;
    part_config.open(partition);
    for (unsigned i = 0; i < partition_config.size(); ++i)
      part_config << partition_config.at(i) << endl;
    part_config.close();
  }
  if (comp_partition_config.size() != 0)
  {
    string complete_partition(bench);
    complete_partition += "_complete_partition_config";

    ofstream comp_config;
    comp_config.open(complete_partition);
    for (unsigned i = 0; i < comp_partition_config.size(); ++i)
      comp_config << comp_partition_config.at(i) << endl;
    comp_config.close();
  }
}

bool fileExists (const string file_name)
{
  struct stat buf;
  if (stat(file_name.c_str(), &buf) != -1)
    return true;
  return false;
}

