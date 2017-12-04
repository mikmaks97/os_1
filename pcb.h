#ifndef PCB_H
#define PCB_H

#include <string>
#include <cmath>

// Process Control Block struct with all process information
struct PCB {
  enum state {active, waiting};  // unused
  size_t pid;
  int size;
  static int page_size;
  int pages;
  std::vector<int> page_table;

  std::string file_name;
  size_t start_mem_loc, physical_loc;
  char op;
  size_t file_size;
  // disks
  int cylinder_num;

  float cpu_time = 0;
  int bursts = 0;

  PCB(size_t new_pid, int new_size) : pid{new_pid}, size{new_size},
                                      file_name{""}, start_mem_loc{0},
                                      physical_loc{0}, op{'-'},
                                      file_size{0}, cylinder_num{-1} {
                                        page_table.resize(ceil((float)size/(float)page_size));
                                      }
};

#endif
