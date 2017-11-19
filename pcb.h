#ifndef PCB_H
#define PCB_H

#include <string>

struct PCB {
  enum state {active, waiting};  // unused
  size_t pid;

  std::string file_name;
  size_t start_mem_loc;
  char op;
  size_t file_size;
  // disks
  int cylinder_num;

  float cpu_time = 0;
  int bursts = 0;

  PCB(size_t new_pid) : pid{new_pid}, file_name{""},
                        start_mem_loc{0}, op{'-'}, file_size{0},
                        cylinder_num{-1} {}
};

#endif
