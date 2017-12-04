// Maximilian Ioffe
// 08 Oct 2017
// Implementation file for methods and function of basic OS in os.h

#include <iomanip>
#include <limits>
#include <iostream>
#include <sstream>

#include "os.h"

int PCB::page_size = 0;
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
                             CPU_time_sum{other.CPU_time_sum},
                             num_of_completed{other.num_of_completed},
                             time_slice_length{other.time_slice_length},
                             page_size{other.page_size}, mem_size{other.mem_size},
                             max_proc_size{other.max_proc_size},
                             free_frame_list{std::move(other.free_frame_list)},
                             frame_table{std::move(other.frame_table)},
                             devices{std::move(other.devices)},
                             ready_queue{std::move(other.ready_queue)},
                             input_queue{std::move(other.input_queue)} {
    other.active_process = nullptr;
  }
  OS::~OS() {
    for (auto& d: devices) delete d;
    for (auto& p: ready_queue) delete p;
    delete active_process;
  }

  void OS::IORequest(char device_type, int device_num) {
    if (active_process == nullptr) {
      std::cerr << "I/O request failed. CPU has no active process" << std::endl;
      return;
    }

    // add CPU time to process
    active_process->cpu_time += TimeSliceInterrupt();
    active_process->bursts++;

    // calculate which device IO request is meant for
    // convert lowercase device type (c/d/p) to 0,1,2
    int device_id = (device_type-99) % 11;
    int offset = cd_num*(--device_id >= 0);
    offset += disk_num*(device_id-1 >= 0);
    int index = offset + device_num-1;

    // get all IO request information
    std::cout << "File name (max 20 characters): ";
    std::string file_name = InputWithTypeCheck<std::string>("File name invalid");
    while (file_name.size() > 20) {
      std::cout << "File name too large. Try again: ";
      file_name = InputWithTypeCheck<std::string>("File name invalid");
    }

    int start_mem_loc = -1;
    bool invalid = true;
    std::cout << "Starting location in memory (hex): ";
    while (invalid) {
      std::string start_mem_loc_str = InputWithTypeCheck<std::string>("Memory location invalid");
      // make sure memory input is valid hex
      while (start_mem_loc_str.find_first_not_of("012345679abcdefABCDEF") != std::string::npos) {
        start_mem_loc_str = InputWithTypeCheck<std::string>("Memory location invalid");
      }
      std::istringstream ss{start_mem_loc_str};
      ss >> std::hex >> start_mem_loc;
      if (start_mem_loc > 0 && start_mem_loc < active_process->size) {
        invalid = false;
      }
      else {
        std::cout << "Start memory location must be >= 0 and < " << active_process->size << ": ";
      }
    }
    // split input logical address into page number and displacement
    int displacement = start_mem_loc % page_size;
    int page_number = start_mem_loc/page_size;
    // get frame number from page table
    int frame_number = active_process->page_table[page_number];
    active_process->physical_loc = frame_number*page_size + displacement;
    std::cout << "Physical address: " << std::hex << active_process->physical_loc << std::endl;

    char operation = 'w';
    if (device_type != 'p') {  // if device is a printer, only write operation
      std::cout << "Read or write (r/w): ";
      operation = InputWithTypeCheck<char>("Operation invalid (r/w)");
      while (operation != 'r' && operation != 'w') {
        std::cout << "Operation has to be read(r) or write(w): ";
        operation = InputWithTypeCheck<char>("Operation invalid (r/w)");
      }
    }

    int cylinder = -1;
    if (device_type == 'd') {  // if device is disk, ask for cylinder number
      Disk* d = static_cast<Disk*>(devices[index]);
      std::cout << "Cylinder to access: ";
      cylinder = InputWithTypeCheck<int>("Cylinder number invalid");
      while (cylinder < 0 || cylinder > d->num_of_cylinders-1) {
        std::cout << "Cylinder number must be 0-"
                  << d->num_of_cylinders - 1
                  << ": ";
        cylinder = InputWithTypeCheck<int>("Cylinder number invalid");
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
    active_process->cylinder_num = cylinder;

    // push process onto the device queue
    devices[index]->AddRequest(active_process);

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

    PCB* finished = devices[index]->PopFinished();
    if (finished == nullptr) {
      std::cerr << "Device queue empty." << std::endl;
      return;
    }

    // move process for which I/O finished to ready queue or directly to CPU
    if (active_process == nullptr) {
      active_process = finished;
    }
    else {
      ready_queue.push_back(finished);
    }
    std::cout << "IO request for process " << finished->pid << " completed"
              << std::endl;
  }

  void OS::NewProcess() {
    size_t pid = pid_count++;
    std::cout << "Process " << pid << " arrived" << std::endl;
    std::cout << "Process size: ";
    int proc_size = InputWithTypeCheck<int>("Process size invalid: ");
    while (proc_size <= 0) {
      std::cout << "Process size must be > 0: ";
      proc_size = InputWithTypeCheck<int>("Process size invalid: ");
    }
    if (proc_size > max_proc_size) {
      std::cerr << "Rejecting process with size (" << proc_size << ") larger than maximum process size ("
                << max_proc_size << ")" << std::endl;
      return;
    }
    PCB *new_process = new PCB{pid, proc_size};
    DispatchProcess(new_process);
  }

  void OS::DispatchProcess(PCB* p) {
    // if the there are enough frames for the process add it to memory
    if (p->page_table.size() <= free_frame_list.size()) {
      // allocate frames to process
      for (int i = 0; i < p->page_table.size(); i++) {
        int new_frame = free_frame_list.back();
        free_frame_list.pop_back();
        p->page_table[i] = new_frame;
        frame_table[new_frame] = std::make_pair(p->pid, i);
      }

      // give process to CPU or put in ready queue
      if (active_process == nullptr) {
        active_process = p;
      }
      else {
        ready_queue.push_back(p);
      }
    }
    // otherwise add it to the job pool
    else {
      input_queue.insert(p);
    }
  }

  int OS::TimeSliceInterrupt() {
    std::cout << "Duration of time slice process was in the CPU: ";
    int duration = InputWithTypeCheck<int>("Duration invalid: ");
    while (duration < 0 || duration > time_slice_length) {
      std::cout << "Duration must be 0-" << time_slice_length << ": ";
      duration = InputWithTypeCheck<int>("Duration invalid: ");
    }
    return duration;
  }

  void OS::EndOfTimeSlice() {
    if (active_process == nullptr) {
      std::cerr << "No active process" << std::endl;
      return;
    }
    // increment CPU time by time slice length and context switch
    active_process->cpu_time += time_slice_length;
    ready_queue.push_back(active_process);
    active_process = ready_queue.front();
    ready_queue.pop_front();
  }

  void OS::Kill(int proc_id, bool terminated) {
    PCB* kill_proc = nullptr;
    bool stalled = false;
    // check CPU
    if (active_process != nullptr && active_process->pid == proc_id) {
      kill_proc = active_process;
    }
    // check ready queue
    if (kill_proc == nullptr) {
      for (auto itr = ready_queue.begin(); itr != ready_queue.end();) {
        if ((*itr)->pid == proc_id) {
          kill_proc = *itr;
          itr = ready_queue.erase(itr);
          break;
        }
        else ++itr;
      }
    }
    // check device queues
    if (kill_proc == nullptr) {
      bool found = false;
      for (const auto& d: devices) {
        for (const auto& p: d->AllRequests()) {
          if (p->pid == proc_id) {
            kill_proc = p;
            d->RemoveRequest(proc_id);
            stalled = true;
            found = true;
            break;
          }
        }
        if (found) break;
      }
    }
    bool job_pool = false;
    // check job pool
    if (kill_proc == nullptr) {
      for (auto itr = input_queue.begin(); itr != input_queue.end();) {
        if ((*itr)->pid == proc_id) {
          kill_proc = *itr;
          itr = input_queue.erase(itr);
          job_pool = true;
          break;
        }
        else ++itr;
      }
    }
    if (kill_proc == nullptr) {
      std::cerr << "Process with pid " << proc_id << " does not exist" << std::endl;
      return;
    }

    // accounting info
    CPU_time_sum += kill_proc->cpu_time;
    if (!stalled) {
      kill_proc->bursts++;
    }
    if (terminated) {
      num_of_completed++;
    }
    float avg_burst_time = (kill_proc->bursts == 0) ? 0 : kill_proc->cpu_time/kill_proc->bursts;
    std::cout << "Process " << kill_proc->pid << " terminated. ";
    std::cout << "Total CPU time: " << kill_proc->cpu_time
              << ", avg. burst time: " << avg_burst_time << std::endl;

    // free frames and add back to free frame list if process not in job pool
    if (!job_pool) {
      for (int i = 0; i < kill_proc->page_table.size(); i++) {
        int freed_frame = kill_proc->page_table[i];
        free_frame_list.push_back(freed_frame);
        frame_table[freed_frame] = std::make_pair(-1,-1);
      }
    }
    delete kill_proc;  // reclaim PCB memory

    // add largest processes that can fit in free memory from job pool
    for (auto itr = input_queue.begin(); itr != input_queue.end();) {
      if ((*itr)->page_table.size() <= free_frame_list.size()) {
        DispatchProcess(*itr);
        itr = input_queue.erase(itr);
      }
      else ++itr;
    }
  }

  void OS::TerminateActiveProcess() {
    if (active_process == nullptr) {
      std::cerr << "No active process to terminate" << std::endl;
      return;
    }
    active_process->cpu_time += TimeSliceInterrupt();
    Kill(active_process->pid, true);
    if (ready_queue.size() > 0) {
      active_process = ready_queue.front();
      ready_queue.pop_front();
    }
    else
      active_process = nullptr;
  }

  void OS::Snapshot() const {
    std::cout << "Show r/p/d/c/m/j: ";
    char snap_type = InputWithTypeCheck<char>("Invalid snapshot type (r/p/d/c)");
    while (snap_type != 'r' && snap_type != 'p' && snap_type != 'd' &&
           snap_type != 'c' && snap_type != 'm' && snap_type != 'j') {
      std::cout << "Invalid, not r/p/d/c" << std::endl;
      snap_type = InputWithTypeCheck<char>("Invalid snapshot type (r/p/d/c/m/j)");
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');

    int lines_printed = 0;
    if (snap_type != 'm' && snap_type != 'j') {
      float avg_CPU_time = (num_of_completed == 0) ? 0 : CPU_time_sum/num_of_completed;
      std::cout << "Average CPU time of completed processes: "
                << avg_CPU_time << std::endl;
      lines_printed++;
      std::cout << std::setw(5) <<  std::left << "PID"
                << std::setw(10) << std::left << "Filename"
                << std::setw(9) << std::left << "Logical"
                << std::setw(10) << std::left << "Physical"
                << std::setw(5) <<  std::left << "r/w"
                << std::setw(9) << std::left << "Filelen"
                << std::setw(11) << std::left << "Cylinder#"
                << std::setw(10) << std::left << "CPU time"
                << std::setw(9) <<  std::left << "Avg burst" << std::endl;
      lines_printed++;
    }
    if (snap_type == 'r') {  // ready queue id == 4
      std::cout << "-----Ready queue-----" << std::endl;
      lines_printed++;
      PrintStatus(ready_queue, false, lines_printed);
    }
    else if (snap_type == 'j')  {  // input queue id == 5
      std::cout << "-----Job pool-----" << std::endl;
      lines_printed++;
      std::cout << std::setw(6) << std::left << "PID"
                << std::setw(73) << std::left << "Size"
                << std::endl;
      for (const auto& pcb: input_queue) {
        CheckLines(lines_printed);
        std::cout << std::setw(6) << std::left << pcb->pid;
        std::cout << std::setw(73) << std::left << pcb->size;
        std::cout << std::endl;
        lines_printed++;
      }
    }
    else if (snap_type == 'm') {
      std::cout << "Free frame list: ";
      std::string free_frames;
      for (const auto& frame: free_frame_list) {
        std::cout << frame << " ";
      }
      std::cout << std::endl;
      lines_printed++;
      CheckLines(lines_printed);
      std::cout << "-----Frame table----" << std::endl;
      lines_printed++;
      CheckLines(lines_printed);
      std::cout << std::setw(13) << std::left << "Frame num"
                << std::setw(33) << std::left << "PID"
                << std::setw(33) << std::left << "Page num"
                << std::endl;
      lines_printed++;
      CheckLines(lines_printed);
      for (int i = 0; i < frame_table.size(); i++) {
        CheckLines(lines_printed);
        std::string pid = frame_table[i].first == -1 ? "-" : std::to_string(frame_table[i].first);
        std::string page = frame_table[i].second == -1 ? "-" : std::to_string(frame_table[i].second);
        std::cout << std::setw(13) << std::left << i
                  << std::setw(33) << std::left << pid
                  << std::setw(33) << std::left << page
                  << std::endl;
        lines_printed++;
      }
    }
    else {
      // calculate which devices to print contents of
      int start = 0, end = printer_num + disk_num + cd_num;
      if (snap_type == 'c') {
        end = cd_num;
      }
      else if (snap_type == 'd') {
        start = cd_num;
        end = cd_num + disk_num;
      }
      else if (snap_type == 'p') {
        start = cd_num + disk_num;
      }
      for (int i = start; i < end; i++) {
        std::cout << "-----" << snap_type << (i+1)-start << "-----" << std::endl;
        lines_printed++;
        PrintStatus(devices[i]->AllRequests(), true, lines_printed);
      }
    }
  }

  void OS::PrintStatus(const std::deque<PCB*>& req_queue, bool print_props,
                       int& lines_printed) const {
    for (const auto& pcb: req_queue) {
      CheckLines(lines_printed);
      std::cout << std::setw(5) << std::left << pcb->pid;
      float avg_burst_time = (pcb->bursts == 0) ? 0 : pcb->cpu_time/pcb->bursts;
      if (print_props) {
        std::string file_size_out = (pcb->op == 'r') ? "-" : std::to_string(pcb->file_size);
        std::string cylinder_num = (pcb->cylinder_num < 0) ? "-" : std::to_string(pcb->cylinder_num);
        std::cout << std::setw(10) << std::left << pcb->file_name
                  << std::setw(9) << std::left << std::hex << pcb->start_mem_loc
                  << std::setw(10) << std::left << std::hex << pcb->physical_loc
                  << std::setw(5) <<  std::left << pcb->op
                  << std::setw(9) << std::left << file_size_out
                  << std::setw(11) << std::left << cylinder_num
                  << std::setw(10) << std::left << pcb->cpu_time
                  << std::setw(9) <<  std::left << avg_burst_time;
      }
      else {
        std::cout << std::setw(55) << " "
                  << std::setw(10) << std::left << pcb->cpu_time
                  << std::setw(9)  << std::left << avg_burst_time;
      }
      std::cout << std::endl;
      lines_printed++;
      CheckLines(lines_printed);
      std::cout << "Page table: ";
      lines_printed++;
      CheckLines(lines_printed);
      for (const auto& page: pcb->page_table) {
        std::cout << page << " ";
      }
      std::cout << std::endl;
      lines_printed++;
    }
  }

  void OS::CheckLines(int& lines_printed) const {
    if (lines_printed >= 22) {
      std::cout << "Press ENTER to continue output";
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
      lines_printed = 0;
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
    std::vector<int> cyl_nums;
    for (int i = 0; i < disk_num; i++) {
      std::cout << "Num of disk " << i+1 << " cylinders: ";
      int num_of_cylinders = InputWithTypeCheck<int>("Invalid number of cylinders");
      while (num_of_cylinders < 0) {
        std::cout << "Number of cylinders must be >= 0: ";
        num_of_cylinders = InputWithTypeCheck<int>("Invalid number of cylinders");
      }
      cyl_nums.push_back(num_of_cylinders);
    }

    std::cout << "Num of cd-rw devices: ";
    int cd_num = InputWithTypeCheck<int>("Invalid number of cd-rw devices");
    while (cd_num < 0) {
      std::cout << "Number of cd-rw devices must be >= 0: ";
      cd_num = InputWithTypeCheck<int>("Invalid number of cd-rw devices");
    }

    std::cout << "Length of time slice (ms): ";
    int time_slice = InputWithTypeCheck<int>("Invalid time slice length");
    while (time_slice <= 0) {
      std::cout << "Length of time slice must be > 0: ";
      cd_num = InputWithTypeCheck<int>("Invalid time slice length");
    }

    std::cout << "Page size: ";
    int page_size = InputWithTypeCheck<int>("Invalid page size");
    while (page_size <= 0 || (page_size & (~page_size+1)) != page_size) {
      std::cout << "Page size must be > 0 and a power of 2: ";
      page_size = InputWithTypeCheck<int>("Invalid page size");
    }

    std::cout << "Size of memory: ";
    int memory_size = InputWithTypeCheck<int>("Invalid memory size");
    while (memory_size <= 0 || memory_size % page_size != 0) {
      std::cout << "Memory size must be > 0 and multiple of page size: ";
      memory_size = InputWithTypeCheck<int>("Invalid memory size");
    }

    std::cout << "Max process size: ";
    int max_proc_size = InputWithTypeCheck<int>("Invalid max process size");
    while (max_proc_size <= 0 || max_proc_size > memory_size) {
      std::cout << "Max process size must be > 0 and <= " << memory_size << ": ";
      max_proc_size = InputWithTypeCheck<int>("Invalid max process size");
    }

    return OS{printer_num, disk_num, cd_num, time_slice, cyl_nums,
              page_size, memory_size, max_proc_size};
  }

}
