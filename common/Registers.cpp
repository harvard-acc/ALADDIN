#include <cassert>

#include "power_delay.h"
#include "Registers.h"

Registers::~Registers()
{
  for (auto it = regs.begin(); it != regs.end(); ++it)
  {
    delete it->second;
  }
}

void Registers::createRegister(
    std::string baseName, unsigned int num_bytes, float cycleTime)
{
  assert(regs.find(baseName) == regs.end());
  unsigned int new_id = regs.size();
  regs[baseName] = new Register(baseName, num_bytes, cycleTime, new_id);
}

void Registers::getRegisterNames(std::vector<std::string> &names)
{
  for (auto it = regs.begin(); it != regs.end(); it++)
    names.push_back(it->first);
}

bool Registers::has(std::string baseName)
{
  return (regs.find(baseName) != regs.end());
}

double Registers::getArea(std::string baseName)
{
  return regs[baseName]->getArea();
}

double Registers::getReadEnergy(std::string baseName)
{
  return regs[baseName]->getReadEnergy();
}

double Registers::getWriteEnergy(std::string baseName)
{
  return regs[baseName]->getWriteEnergy();
}

double Registers::getLeakagePower(std::string baseName)
{
  return regs[baseName]->getLeakagePower();
}
