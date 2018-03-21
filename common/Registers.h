#ifndef __REGISTERS_H__
#define __REGISTERS_H__

#include <map>
#include <string>
#include <vector>
#include "power_func.h"

class Register {
 public:
  Register(std::string baseName,
           unsigned int _total_size,
           unsigned int _word_size,
           float cycleTime,
           unsigned int id) {
    this->baseName = baseName;
    this->cycleTime = cycleTime;
    this->id = id;
    total_size = _total_size;
    word_size = _word_size;
    loads = 0;
    stores = 0;
    // Units are confusing...they are...
    // REG_int_power is mW, cycleTime is ns, and we want energy in pJ.
    float reg_int_power, reg_sw_power, reg_leak_power, reg_area;
    getRegisterPowerArea(
        cycleTime, &reg_int_power, &reg_sw_power, &reg_leak_power, &reg_area);
    // The read/write energy value should be for a single-word access. Vector
    // accesses should multiply this by the vector size.
    readEnergy = word_size * 8 * (reg_int_power + reg_sw_power) * cycleTime;
    writeEnergy = word_size * 8 * (reg_int_power + reg_sw_power) * cycleTime;
    // REG_leak_power is mW and we want leakPower in mW so we're fine.
    leakPower = total_size * 8 * (reg_leak_power);
    area = total_size * 8 * (reg_area);
  }

  void increment_loads() { loads++; }
  void increment_stores() { stores++; }
  void increment_dma_accesses(bool isLoad) {
    // A dmaLoad will cause a STORE to the reg, and vice versa.
    if (isLoad)
      increment_stores();
    else
      increment_loads();
  }

  double getReadEnergy() { return readEnergy; }
  double getWriteEnergy() { return writeEnergy; }
  double getLeakagePower() { return leakPower; }
  double getArea() { return area; }
  unsigned getTotalSize() const { return total_size; }
  unsigned getWordSize() const { return word_size; }

 private:
  std::string baseName;
  unsigned int total_size;  // bytes
  unsigned int word_size;  // bytes
  unsigned int id;
  float cycleTime;
  double readEnergy;
  double writeEnergy;
  double leakPower;
  double area;

  unsigned int loads;
  unsigned int stores;
};

// TODO: This is a pretty terrible naming distinction...
class Registers {

 public:
  Registers() {}
  ~Registers();

  void createRegister(std::string baseName,
                      unsigned total_size_bytes,
                      unsigned word_size_bytes,
                      float cycleTime);
  void getRegisterNames(std::vector<std::string>& names);
  Register* getRegister(std::string baseName) { return regs[baseName]; }
  double getArea(std::string baseName);
  double getTotalArea();
  double getLeakagePower(std::string baseName);
  double getTotalLeakagePower();
  double getReadEnergy(std::string baseName);
  double getWriteEnergy(std::string baseName);
  bool has(std::string baseName);
  void clear();

 private:
  std::map<std::string, Register*> regs;
};

#endif
