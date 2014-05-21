#include "file_func.h"

void init_method_order(string bench, vector<string> &v_method_order,
  unordered_map<string, int> &map_method_2_callinst)
{
  ifstream method_order_file;
  method_order_file.open(bench + "_method_order");
  while (!method_order_file.eof())
  {
    string wholeline;
    getline(method_order_file, wholeline);
    if (wholeline.size() == 0)
      break;
    int pos = 0;
    pos = wholeline.find(",");
    string current_method = wholeline.substr(0,pos);
    wholeline.erase(0, pos+1);
    
    pos = wholeline.find(",");
    string next_method = wholeline.substr(0,pos);
    int call_inst = atoi(wholeline.substr(pos+1).c_str());
    
    v_method_order.push_back(current_method);
    map_method_2_callinst[current_method] = call_inst;
  }
  v_method_order.push_back(bench);
  method_order_file.close();
}

void read_file(string file_name, vector<int> &output)
{
  ifstream file;
  cerr << file_name << endl;
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

void read_string_file(string file_name, vector<string> &output)
{
  ifstream file;
  cerr << file_name << endl;
  file.open(file_name.c_str());
  string wholeline;
  if (file.is_open())
  {
    while (!file.eof())
    {
      getline(file, wholeline);
      if (wholeline.size() == 0)
        break;
      output.push_back(wholeline);
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
  igzstream gzip_file;
  gzip_file.open(gzip_file_name.c_str());
  cerr << gzip_file_name << endl;
  unsigned i = 0;
  string wholeline;
  while(!gzip_file.eof() && i< size)
  {
    wholeline.clear();
    getline(gzip_file, wholeline);
    output.at(i) = wholeline;
    i++;
  }
  gzip_file.close();
  if (i == 0)
  {
    cerr << "file not open " << gzip_file_name << endl;
    exit(0);
  }
}

void read_gzip_float_file(string gzip_file_name, unsigned size, vector<float> &output)
{
  igzstream gzip_file;
  gzip_file.open(gzip_file_name.c_str());
  cerr << gzip_file_name << endl;
  unsigned i = 0;
  string wholeline;
  while(!gzip_file.eof() && i< size)
  {
    wholeline.clear();
    getline(gzip_file, wholeline);
    output.at(i) = atof(wholeline.c_str());
    i++;
  }
  gzip_file.close();
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
  igzstream gzip_file;
  cerr << gzip_file_name << endl;
  if (fileExists(gzip_file_name))
  {
    gzip_file.open(gzip_file_name.c_str());
    unsigned i = 0;
    string wholeline;
    while(!gzip_file.eof() && i< size)
    {
      wholeline.clear();
      getline(gzip_file, wholeline);
      //assert(wholeline.size() != 0);
      output.at(i) = atoi(wholeline.c_str());
      i++;
    }
    gzip_file.close();
    cerr << "end of reading file" << gzip_file_name << endl;
  }
  else
  {
    cerr << "no such file" << endl;
  }
}

void read_gzip_unsigned_file(string gzip_file_name, unsigned size,
  vector<unsigned> &output)
{
  igzstream gzip_file;
  cerr << gzip_file_name << endl;
  gzip_file.open(gzip_file_name.c_str());
  unsigned i = 0;
  string wholeline;
  while(!gzip_file.eof() && i< size)
  {
    wholeline.clear();
    getline(gzip_file, wholeline);
    //assert(wholeline.size() != 0);
    output.at(i) = (unsigned)atoi(wholeline.c_str());
    i++;
  }
  gzip_file.close();
}

/*Read gz file into vector
Input: gzip-file-name, size of elements, vector to write to
*/
void read_gzip_file_no_size(string gzip_file_name, vector<int> &output)
{
  igzstream gzip_file;
  cerr << gzip_file_name << endl;
  gzip_file.open(gzip_file_name.c_str());
  unsigned i = 0;
  string wholeline;
  while(!gzip_file.eof())
  {
    wholeline.clear();
    getline(gzip_file, wholeline);
    output.push_back(atoi(wholeline.c_str()));
    i++;
  }
  gzip_file.close();
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
  igzstream gzip_file;
  cerr << gzip_file_name << endl;
  gzip_file.open(gzip_file_name.c_str());
  unsigned i = 0;
  string wholeline;
  while(!gzip_file.eof() && i< size)
  {
    wholeline.clear();
    getline(gzip_file, wholeline);
    if (wholeline.size() == 0)
      break;
    istringstream parser(wholeline, istringstream::in);
    unsigned element1, element2;
    char comma;
    parser >> element1 >> comma >> element2;
    output.at(i).first = element1;
    output.at(i).second = element2;
    i++;
  }
  gzip_file.close();
  if (i == 0)
  {
    cerr << "file not open " << gzip_file_name << endl;
    exit(0);
  }
}

void read_gzip_1in2_unsigned_file(string gzip_file_name, unsigned size,
  vector<unsigned> &output)
{
  igzstream gzip_file;
  cerr << gzip_file_name << "," << size << endl;
  gzip_file.open(gzip_file_name.c_str());
  unsigned i = 0;
  string wholeline;
  while(!gzip_file.eof() && i< size)
  {
    wholeline.clear();
    getline(gzip_file, wholeline);
    if (wholeline.size() == 0)
      break;
    istringstream parser(wholeline, istringstream::in);
    unsigned element1, element2;
    char comma;
    parser >> element1 >> comma >> element2;
    output.at(i) = element1;
    i++;
  }
  gzip_file.close();
  if (i == 0)
  {
    cerr << "file not open " << gzip_file_name << endl;
    exit(0);
  }
}

void write_file(string file_name, unsigned size, vector<int> &output)
{
  ofstream file;
  file.open(file_name.c_str());
  for (unsigned i = 0; i < size; ++i)
  {
    file << output.at(i) << endl;
  }
  file.close();
}

void write_string_file(string file_name, unsigned size, vector<string> &output)
{
  ofstream file;
  file.open(file_name.c_str());
  for (unsigned i = 0; i < size; ++i)
  {
    file << output.at(i) << endl;
  }
  file.close();
}

void write_gzip_file(string gzip_file_name, unsigned size, vector<int> &output)
{
  ogzstream gzip_file;
  cerr << gzip_file_name << endl;
  gzip_file.open(gzip_file_name.c_str());
  for (unsigned i = 0; i < size; ++i)
    gzip_file << output.at(i) << endl;
  gzip_file.close();
}

void write_gzip_bool_file(string gzip_file_name, unsigned size, vector<bool> &output)
{
  ogzstream gzip_file;
  cerr << gzip_file_name << endl;
  gzip_file.open(gzip_file_name.c_str());
  for (unsigned i = 0; i < size; ++i)
    gzip_file << output.at(i) << endl;
  gzip_file.close();
}

void write_gzip_unsigned_file(string gzip_file_name, unsigned size,
vector<unsigned> &output)
{
  ogzstream gzip_file;
  cerr << gzip_file_name << endl;
  gzip_file.open(gzip_file_name.c_str());
  for (unsigned i = 0; i < size; ++i)
    gzip_file << output.at(i) << endl;
  gzip_file.close();
}

void write_gzip_float_file(string gzip_file_name, unsigned size, vector<float> &output)
{
  ogzstream gzip_file;
  gzip_file.open(gzip_file_name.c_str());
  cerr << gzip_file_name << endl;
  for (unsigned i = 0; i < size; ++i)
  {
    gzip_file << output.at(i) << endl;
  }
  gzip_file.close();
}

void write_gzip_string_file(string gzip_file_name, unsigned size, vector<string> &output)
{
  ogzstream gzip_file;
  gzip_file.open(gzip_file_name.c_str());
  cerr << gzip_file_name << endl;
  for (unsigned i = 0; i < size; ++i)
  {
    gzip_file << output.at(i) << endl;
  }
  gzip_file.close();
}

void parse_config(string bench, string config_file_name)
{
  ifstream config_file;
  config_file.open(config_file_name);
  string wholeline;

  vector<string> flatten_config;
  vector<string> unrolling_config;
  vector<string> partition_config;
  vector<string> comp_partition_config;

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
    //cerr << type << "," << pos_end_tag << endl;
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

    //else if (!type.compare("ambiguation"))
      //ambiguation_config.push_back(rest_line);
    
   else
    {
      cerr << "what else? " << type << endl;
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
  
  /*
  if (ambiguation_config.size() != 0)
  {
    string file_name(bench);
    file_name += "_ambiguation_config";
    ofstream output;
    output.open(file_name);
    for (unsigned i = 0; i < ambiguation_config.size(); ++i)
      output << ambiguation_config.at(i) << endl;
    output.close();
  }
  if (cache_config.size() != 0)
  {
    string file_name(bench);
    file_name += "_cache_config";
    ofstream output;
    output.open(file_name);
    for (unsigned i = 0; i < cache_config.size(); ++i)
      output << cache_config.at(i) << endl;
    output.close();
  }
  if (forwarding_config.size() != 0)
  {
    string file_name(bench);
    file_name += "_forwarding_config";
    ofstream output;
    output.open(file_name);
    for (unsigned i = 0; i < forwarding_config.size(); ++i)
      output << forwarding_config.at(i) << endl;
    output.close();
  }
  if (mem_config.size() != 0)
  {
    string file_name(bench);
    file_name += "_mem_config";
    ofstream output;
    output.open(file_name);
    for (unsigned i = 0; i < mem_config.size(); ++i)
      output << mem_config.at(i) << endl;
    output.close();
  }
  */
}

bool fileExists (const string file_name)
{
  struct stat buf;
  if (stat(file_name.c_str(), &buf) != -1)
    return true;
  return false;
}

void update_method_latency (string bench, unordered_map<int, int> &call_latency)
{
  std::cerr << "in update_method_latency " << endl;
  string method_latency_file(bench + "_method_latency");
  
  if (fileExists(method_latency_file))
  {
    ifstream method_latency;
    method_latency.open(method_latency_file);
    while(!method_latency.eof())
    {
      string wholeline;
      getline(method_latency, wholeline);

      if (wholeline.size() == 0)
        break;

      int callinst;
      int latency;
      
      istringstream parser(wholeline);
      char comma;
      parser >> callinst >> comma >> latency;

      call_latency[callinst] = latency ;
    }
    method_latency.close();
  }
  std::cerr << "leaving update_method_latency " << endl;
}

