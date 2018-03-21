#include <cassert>

#include "Registers.h"

Registers::~Registers() { clear(); }

void Registers::clear() {
  for (auto it = regs.begin(); it != regs.end(); ++it) {
    delete it->second;
  }
  regs.clear();
}

void Registers::createRegister(std::string baseName,
                               unsigned int total_size_bytes,
                               unsigned int word_size_bytes,
                               float cycleTime) {
  assert(regs.find(baseName) == regs.end());
  unsigned int new_id = regs.size();
  regs[baseName] = new Register(
      baseName, total_size_bytes, word_size_bytes, cycleTime, new_id);
}

void Registers::getRegisterNames(std::vector<std::string>& names) {
  for (auto it = regs.begin(); it != regs.end(); it++)
    names.push_back(it->first);
}

bool Registers::has(std::string baseName) {
  return (regs.find(baseName) != regs.end());
}

double Registers::getArea(std::string baseName) {
  return regs[baseName]->getArea();
}

double Registers::getTotalArea() {
  float total_area = 0;
  for (auto it = regs.begin(); it != regs.end(); it++)
    total_area += getArea(it->first);
  return total_area;
}
double Registers::getTotalLeakagePower() {
  float total_leakage = 0;
  for (auto it = regs.begin(); it != regs.end(); it++)
    total_leakage += getLeakagePower(it->first);
  return total_leakage;
}
double Registers::getReadEnergy(std::string baseName) {
  return regs[baseName]->getReadEnergy();
}

double Registers::getWriteEnergy(std::string baseName) {
  return regs[baseName]->getWriteEnergy();
}

double Registers::getLeakagePower(std::string baseName) {
  return regs[baseName]->getLeakagePower();
}
