#pragma once
#include <cstring>
struct FakeEEPROM {
  unsigned char bytes[1024] = {};
  int writes = 0;
  bool drop_writes = false;
  int bytes_per_write = -1;
  void (*after_put)() = nullptr;
  template<class T> void get(int address, T &value) {
    std::memcpy(&value, bytes + address, sizeof(T));
  }
  template<class T> void put(int address, const T &value) {
    size_t count=sizeof(T);
    if(bytes_per_write>=0 && size_t(bytes_per_write)<count)count=size_t(bytes_per_write);
    if (!drop_writes) std::memcpy(bytes + address, &value, count);
    ++writes;
    if(after_put)after_put();
  }
};
extern FakeEEPROM EEPROM;
