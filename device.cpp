#include "device.h"
#include <exception>

Device::~Device() {}
Device* Device::make_device(char device_type) {
  if (device_type == 'p') return new Printer;
  else if (device_type == 'c') return new CD_RW;
  else if (device_type == 'd') return new Disk;
  else throw std::invalid_argument("device_type");
}


Printer::~Printer() {
  for (auto& p: req_queue) {
    delete p;
  }
}
void Printer::AddRequest(PCB* request) {
  req_queue.push_back(request);
}
PCB* Printer::PopFinished() {
  if (req_queue.empty())
    return nullptr;
  PCB* finished = req_queue.front();
  req_queue.pop_front();
  return finished;
}
void Printer::RemoveRequest(int pid) {
  for (auto itr = req_queue.begin(); itr != req_queue.end();) {
    if ((*itr)->pid == pid) {
      itr = req_queue.erase(itr);
    }
    else ++itr;
  }
}
const std::deque<PCB*> Printer::AllRequests() const {return req_queue;}


CD_RW::~CD_RW() {
  for (auto& p: req_queue) {
    delete p;
  }
}
void CD_RW::AddRequest(PCB* request) {
  req_queue.push_back(request);
}
PCB* CD_RW::PopFinished() {
  if (req_queue.empty())
    return nullptr;
  PCB* finished = req_queue.front();
  req_queue.pop_front();
  return finished;
}
void CD_RW::RemoveRequest(int pid) {
  for (auto itr = req_queue.begin(); itr != req_queue.end();) {
    if ((*itr)->pid == pid) {
      itr = req_queue.erase(itr);
    }
    else ++itr;
  }
}
const std::deque<PCB*> CD_RW::AllRequests() const {return req_queue;}


Disk::~Disk() {
  for (int i = 0; i < queue_1.size(); i++) {
    PCB* p = queue_1.top();
    queue_1.pop(head_pos);
    delete p;
  }
  for (int i = 0; i < queue_2.size(); i++) {
    PCB* p = queue_2.top();
    queue_2.pop(head_pos);
    delete p;
  }
}
void Disk::AddRequest(PCB* request) {
  // add to first queue, then close it
  if (!running) {
    queue_1.push(head_pos, request);
    running = true;
    return;
  }
  // if the first queue is being serviced, defer all requests to second queue
  if (run_queue == 1) {
    queue_2.push(head_pos, request);
  }
  // if the second queue is being serviced, defer all requests to first queue
  else if (run_queue == 2) {
    queue_1.push(head_pos, request);
  }
}
PCB* Disk::PopFinished() {
  PCB* finished = nullptr;
  // pop from first queue if servicing it
  if (run_queue == 1) {
    if (!queue_1.empty()) {
      finished = queue_1.top();
      queue_1.pop(head_pos);
      if (queue_1.empty()) {  // if after pop queue is empty, change service queue
        run_queue = 2;
      }
    }
  }
  else if (run_queue == 2) {
    if (!queue_2.empty()) {
      finished = queue_2.top();
      queue_2.pop(head_pos);
      if (queue_2.empty()) {
        run_queue = 1;
      }
    }
  }
  if (finished != nullptr) {
    head_pos = finished->cylinder_num; // move seek head to new cylinder pos
  }
  return finished;
}
void Disk::RemoveRequest(int pid) {
  bool deleted = false;

  // iterate through queue_1 and look for process with pid == pid
  for (auto itr = queue_1.begin(); itr != queue_1.end();) {
    if ((*itr)->pid == pid) {
      itr = queue_1.erase(itr);
      deleted = true;
      break;
    }
    else ++itr;
  }
  if (deleted && queue_1.empty() && run_queue == 1)
    run_queue = 2;
  bool deleted_2 = false;
  // if not found in queue_1, check queue_2
  if (!deleted) {
    for (auto itr = queue_2.begin(); itr != queue_2.end();) {
      if ((*itr)->pid == pid) {
        itr = queue_2.erase(itr);
        deleted_2 = true;
        break;
      }
      else ++itr;
    }
    if (deleted_2 && queue_2.empty() && run_queue == 2)
      run_queue = 1;
  }
}
const std::deque<PCB*> Disk::AllRequests() const {
  std::deque<PCB*> req_queue;
  // if servicing queue 1 put queue 1 requests first
  if (run_queue == 1) {
    std::vector<PCB*> vec_1 = queue_1.ToVector(head_pos);
    std::vector<PCB*> vec_2 = queue_2.ToVector(head_pos);
    for (const auto& v: vec_1) {
      req_queue.push_back(v);
    }
    for (const auto& v: vec_2) {
      req_queue.push_back(v);
    }
  }
  else if (run_queue == 2) {
    std::vector<PCB*> vec_1 = queue_1.ToVector(head_pos);
    std::vector<PCB*> vec_2 = queue_2.ToVector(head_pos);
    for (const auto& v: vec_2) {
      req_queue.push_back(v);
    }
    for (const auto& v: vec_1) {
      req_queue.push_back(v);
    }
  }
  return req_queue;
}
