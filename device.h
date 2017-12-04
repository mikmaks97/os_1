#ifndef DEVICE_H
#define DEVICE_H

#include <iostream>
#include <deque>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <string>

#include "pcb.h"

// Interface for devices as well as factory method to create specific devices
// AddRequest(PCB* request) - add a request to the device queue
// PopFinished()            - pop a request off the device queue
// RemoveRequest(int pid)   - find and remove request with pid == pid if exists
// AllRequests()            - return all requests as a deque
struct Device {
  static Device* make_device(char device_type);
  virtual ~Device() = 0;
  virtual void AddRequest(PCB* request) = 0;
  virtual PCB* PopFinished() = 0;
  virtual void RemoveRequest(int pid) = 0;
  virtual const std::deque<PCB*> AllRequests() const = 0;
};

// Derived Printer class
struct Printer: Device {
  ~Printer();
  void AddRequest(PCB* request);
  PCB* PopFinished();
  const std::deque<PCB*> AllRequests() const;

  void RemoveRequest(int pid);
  std::deque<PCB*> req_queue;
};

// Derived CD/RW class
struct CD_RW: Device {
  ~CD_RW();
  void AddRequest(PCB* request);
  PCB* PopFinished();
  const std::deque<PCB*> AllRequests() const;

  void RemoveRequest(int pid);
  std::deque<PCB*> req_queue;
};


// Derived Disk class
// Uses FSCAN scheduling algorithm for device queue
struct Disk: Device {
  private:
    bool running = false;
    int run_queue = 1;
    int head_pos = 0;

    // Comparison functor for PCB* type in priority queue
    // Priority: minimum seek time from current head position to PCB cylinder
    struct LessSeekTime {
      LessSeekTime(int h_pos): _head_pos{h_pos} {}
      int _head_pos;
      bool operator()(const PCB* proc1, const PCB* proc2) const {
        return abs(_head_pos-proc2->cylinder_num) < abs(_head_pos-proc1->cylinder_num);
      }
    };

    // Helper priority queue class for PCBs
    class PCBPriorityQueue {
      private:
        std::vector<PCB*> elements;

      public:
        typedef std::vector<PCB*>::iterator iterator;
        typedef std::vector<PCB*>::const_iterator const_iterator;

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

        iterator erase(iterator pos) {return elements.erase(pos);}

        PCB* top() {
          if (!elements.empty()) {
            return elements.front();
          }
          return nullptr;
        }
        int size() {return elements.size();}
        bool empty() {return elements.empty();}

        iterator begin() {return elements.begin();}
        const_iterator begin() const {return elements.begin();}
        iterator end()   {return elements.end();}
        const_iterator end() const {return elements.end();}
    };

  public:
    int num_of_cylinders;
    PCBPriorityQueue queue_1, queue_2;  // run queue and waiting queue

    Disk(): num_of_cylinders{0} {}
    ~Disk();

    void RemoveRequest(int pid);
    void AddRequest(PCB* request);
    PCB* PopFinished();
    const std::deque<PCB*> AllRequests() const;
};

#endif
