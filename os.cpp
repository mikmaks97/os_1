// Maximilian Ioffe
// 08 Oct 2017
// Implementation file for methods and function of basic OS in os.h

#include "os.h"
#include <limits>
#include <iostream>

namespace os_ops {

  // Get input from standard input stream until input type matches type T of
  // variable it is being stored in.
  // Returns retrieved input.
  template<typename T>
  T InputWithTypeCheck(const std::string& error_message) {
    T input_var;
    std::cin >> input_var;
    while (std::cin.fail()) {
      std::cout << error_message << ": ";
      std::cin.clear();
      std::cin.ignore(256,'\n');
      std::cin >> input_var;
    }
    return input_var;
  }

  OS::OS(OS&& other) : active_process{other.active_process},
                             pid_count{other.pid_count},
                             printer_num{other.printer_num},
                             disk_num{other.disk_num}, cd_num{other.cd_num},
                             devices{std::move(other.devices)},
                             ready_queue{std::move(other.ready_queue)} {
    std::cout << "moving" << std::endl;
    other.active_process = nullptr;
    for (auto& d: devices) for (auto &p: d) p = nullptr;
    for (auto& p: ready_queue) p = nullptr;
  }
  OS::~OS() {
    for (auto& d: devices) for (auto &p: d) delete p;
    for (auto& p: ready_queue) delete p;
    delete active_process;
    active_process = nullptr;
  }

  void OS::IORequest(char device_type, int device_num) {
    if (active_process == nullptr) {
      std::cerr << "I/O request failed. CPU has no active process" << std::endl;
      return;
    }
    std::cout << "File name (max 20 characters): ";
    std::string file_name = InputWithTypeCheck<std::string>("File name invalid");
    while (file_name.size() > 20) {
      std::cout << "File name too large. Try again: ";
      file_name = InputWithTypeCheck<std::string>("File name invalid");
    }

    std::cout << "Starting location in memory: ";
    int start_mem_loc = InputWithTypeCheck<int>("Memory location invalid");
    while (start_mem_loc < 0) {
      std::cout << "Start memory location must be >= 0: ";
      start_mem_loc = InputWithTypeCheck<int>("Memory location invalid");
    }

    char operation = 'w';
    if (device_type != 'p') {  // if device is a printer, only write operation
      std::cout << "Read or write (r/w): ";
      operation = InputWithTypeCheck<char>("Operation invalid (r/w)");
      while (operation != 'r' && operation != 'w') {
        std::cout << "Operation has to be read(r) or write(w): ";
        operation = InputWithTypeCheck<char>("Operation invalid (r/w)");
      }
    }

    int file_size = 0;
    if (operation == 'w') {
      std::cout << "Write file size: ";
      file_size = InputWithTypeCheck<int>("File size invalid");
      while (file_size <= 0) {
        std::cout << "File size must be > 0: ";
        file_size = InputWithTypeCheck<int>("File size invalid");
      }
    }
    active_process->file_name = file_name;
    active_process->start_mem_loc = start_mem_loc;
    active_process->op = operation;
    active_process->file_size = file_size;

    // convert ascii device type (c/d/p) to (0/1/2)
    int device_id = (device_type-99) % 11;

    // since all devices are stored in one array offset depends on device type
    int offset = cd_num*(--device_id >= 0);
    offset += disk_num*(device_id-1 >= 0);
    devices[offset + device_num-1].push_back(active_process);

    // move new process from ready queue to CPU
    if (!ready_queue.empty()) {
      active_process = ready_queue.front();
      ready_queue.pop_front();
    }
    else
      active_process = nullptr;
  }

  void OS::HandleInterrupt(char device_type, int device_num) {
    // convert device type (uppercase C/D/P) to 0/1/2
    int device_id = (device_type-67) % 11;
    int offset = cd_num*(--device_id >= 0);
    offset += disk_num*(device_id-1 >= 0);
    int index = offset + device_num-1;

    if (devices[index].empty()) {
      std::cerr << "Device queue empty." << std::endl;
      return;
    }

    // move process for which I/O finished to ready queue or directly to CPU
    if (active_process == nullptr) {
      active_process = devices[index].front();
    }
    else {
      ready_queue.push_back(devices[index].front());
    }
    std::cout << "IO request for process " << devices[index].front()->pid
              << " completed" << std::endl;
    devices[index].pop_front();
  }

  void OS::NewProcess() {
    PCB *new_process = new PCB{pid_count++};
    std::cout << "Process " << new_process->pid << " arrived" << std::endl;
    if (active_process == nullptr) {
      active_process = new_process;
    }
    else {
      ready_queue.push_back(new_process);
    }
  }
  void OS::TerminateActiveProcess() {
    if (active_process == nullptr) {
      std::cerr << "No active process to terminate" << std::endl;
      return;
    }
    std::cout << "Process " << active_process->pid << " terminated" << std::endl;
    delete active_process;  // reclaim PCB memory
    if (ready_queue.size() > 0) {
      active_process = ready_queue.front();
      ready_queue.pop_front();
    }
    else
      active_process = nullptr;
  }

  void OS::Snapshot() const {
    std::cout << "Show r/p/d/c: ";
    char snap_type = InputWithTypeCheck<char>("Invalid snapshot type (r/p/d/c)");
    while (snap_type != 'r' && snap_type != 'p' &&
           snap_type != 'd' && snap_type != 'c') {
      std::cout << "Invalid, not r/p/d/c" << std::endl;
      snap_type = InputWithTypeCheck<char>("Invalid snapshot type (r/p/d/c)");
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');

    std::cout << "PID\t" << "File name\t\t" << "Mem start\t"
              << "r/w\t" << "file length" << std::endl;
    int lines_printed = 1;
    char device_type = (snap_type-99) % 11;
    if (device_type == 4) {  // ready queue id == 4
      std::cout << "-----Ready queue-----" << std::endl;
      lines_printed++;
      PrintStatus(ready_queue, false, lines_printed);
    }
    else {
      // calculate which devices to print contents of
      int start = 0, end = printer_num + disk_num + cd_num;
      if (device_type == 0) {
        end = cd_num;
      }
      else if (device_type == 1) {
        start = cd_num;
        end = cd_num + disk_num;
      }
      else if (device_type == 2) {
        start = cd_num + disk_num;
      }
      for (int i = start; i < end; i++) {
        std::cout << "-----" << snap_type << (i+1)-start << "-----" << std::endl;
        lines_printed++;
        PrintStatus(devices[i], true, lines_printed);
      }
    }

  }

  void OS::PrintStatus(const std::deque<PCB*>& device, bool print_props, int& lines_printed) const {
    for (const auto& pcb: device) {
      if (lines_printed >= 22) {
        std::cout << "Press ENTER to continue output";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
        lines_printed = 0;
      }
      std::cout << pcb->pid;
      std::string file_size_out = (pcb->op == 'r') ? "-" : std::to_string(pcb->file_size);
      if (print_props) {
        printf("\t%-24s%-16s%c\t%s", pcb->file_name.c_str(),
               std::to_string(pcb->start_mem_loc).c_str(), pcb->op,
               file_size_out.c_str());
      }
      std::cout << std::endl;
      lines_printed++;
    }
  }

  OS Sysgen() {
    std::cout << "SYSGEN" << std::endl;
    std::cout << "Num of printer devices: ";
    int printer_num = InputWithTypeCheck<int>("Invalid number of printers");
    while (printer_num < 0) {
      std::cout << "Number of printers must be >= 0: ";
      printer_num = InputWithTypeCheck<int>("Invalid number of printers");
    }

    std::cout << "Num of disk devices: ";
    int disk_num = InputWithTypeCheck<int>("Invalid number of disk devices");
    while (disk_num < 0) {
      std::cout << "Number of disk devices must be >= 0: ";
      disk_num = InputWithTypeCheck<int>("Invalid number of disk devices");
    }

    std::cout << "Num of cd-rw devices: ";
    int cd_num = InputWithTypeCheck<int>("Invalid number of cd-rw devices");
    while (cd_num < 0) {
      std::cout << "Number of cd-rw devices must be >= 0: ";
      cd_num = InputWithTypeCheck<int>("Invalid number of cd-rw devices");
    }

    return OS{printer_num, disk_num, cd_num};
  }

}
