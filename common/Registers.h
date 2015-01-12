#ifndef __REGISTERS_H__
#define __REGISTERS_H__

#include <map>
#include <string>
#include <vector>

class Register
{
  public:
    Register(std::string baseName, unsigned int size,
             float cycleTime, unsigned int id)
    {
      this->baseName = baseName;
      this->cycleTime = cycleTime;
      this->id = id;
      num_bytes = size;
      loads = 0;
      stores = 0;
      // Units are confusing.
      // REG_int_power is mW, cycleTime is ns, and we want energy in nJ, so we
      // have to convert mW to W by dividing by 1000.
      readEnergy = num_bytes*8*(REG_int_power+REG_sw_power)*cycleTime/1000;
      writeEnergy = num_bytes*8*(REG_int_power+REG_sw_power)*cycleTime/1000;
      // REG_leak_power is mW and we want leakPower in mW so we're fine.
      leakPower = num_bytes * 8 * (REG_leak_power);
      area = num_bytes * 8 * (REG_area);
    }

    void increment_loads() { loads++; }
    void increment_stores() { stores++; }

    double getReadEnergy() { return readEnergy; }
    double getWriteEnergy() { return writeEnergy; }
    double getLeakagePower() { return leakPower; }
    double getArea() { return area; }

  private:
    std::string baseName;
    unsigned int num_bytes;
    unsigned int id;
    float cycleTime;
    double readEnergy;
    double writeEnergy;
    double leakPower;
    double area;

    unsigned int loads;
    unsigned int stores;
};

class Registers
{

  public:
    Registers() {}
    ~Registers();

    void createRegister(
        std::string baseName, unsigned num_bytes, float cycleTime);
    void getRegisterNames(std::vector<std::string> &names);
    Register* getRegister(std::string baseName) { return regs[baseName]; }
    double getArea(std::string baseName);
    double getLeakagePower(std::string baseName);
    double getReadEnergy(std::string baseName);
    double getWriteEnergy(std::string baseName);
    bool has(std::string baseName);

  private:
    std::map<std::string, Register*> regs;

};

#endif
