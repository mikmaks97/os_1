#ifndef DEVICE_H
#define DEVICE_H

#include <iostream>
#include <deque>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include "pcb.h"
#include <string>

namespace {
}

struct Device {
  static Device* make_device(char device_type);
  virtual ~Device() = 0;
  virtual void AddRequest(PCB* request) = 0;
  virtual PCB* PopFinished() = 0;
  virtual const std::deque<PCB*> AllRequests() const = 0;
};

struct Printer: Device {
  ~Printer();
  void AddRequest(PCB* request);
  PCB* PopFinished();
  const std::deque<PCB*> AllRequests() const;

  std::deque<PCB*> req_queue;
};

struct CD_RW: Device {
  ~CD_RW();
  void AddRequest(PCB* request);
  PCB* PopFinished();
  const std::deque<PCB*> AllRequests() const;

  std::deque<PCB*> req_queue;
};

struct Disk: Device {
  private:
    bool running = false;
    int run_queue = 1;
    int head_pos = 0;

    struct LessSeekTime {
      LessSeekTime(int h_pos): _head_pos{h_pos} {}
      int _head_pos;
      bool operator()(const PCB* proc1, const PCB* proc2) const {
        return abs(_head_pos-proc2->cylinder_num) < abs(_head_pos-proc1->cylinder_num);
      }
    };

    class PCBPriorityQueue {
      private:
        std::vector<PCB*> elements;

      public:
        std::vector<PCB*> ToVector(int h_pos) const {
          std::vector<PCB*> elems = elements;
          std::vector<PCB*> new_vec;
          while (!elems.empty()) {
            new_vec.push_back(elems.front());
            std::pop_heap(elems.begin(), elems.end(), LessSeekTime(h_pos));
            elems.pop_back();
          }
          return new_vec;
        }
        void push(int h_pos, PCB* e) {
          elements.push_back(e);
          std::push_heap(elements.begin(), elements.end(), LessSeekTime(h_pos));
        }
        void pop(int h_pos) {
          if (!elements.empty()) {
            std::pop_heap(elements.begin(), elements.end(), LessSeekTime(h_pos));
            elements.pop_back();
          }
        }
        PCB* top() {
          if (!elements.empty()) {
            return elements.front();
          }
          return nullptr;
        }
        int size() {return elements.size();}
        bool empty() {return elements.empty();}

        typedef std::vector<PCB*>::iterator iterator;
        typedef std::vector<PCB*>::const_iterator const_iterator;
        iterator begin() {return elements.begin();}
        const_iterator begin() const {return elements.begin();}
        iterator end()   {return elements.end();}
        const_iterator end() const {return elements.end();}
    };

  public:
    int num_of_cylinders;
    PCBPriorityQueue queue_1, queue_2;

    Disk(): num_of_cylinders{0} {}
    ~Disk();

    void AddRequest(PCB* request);
    PCB* PopFinished();
    const std::deque<PCB*> AllRequests() const;
};

#endif
