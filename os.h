// Maximilian Ioffe
// 8 Oct 2017
// A basic model of an operating system that moves processes between device
// queues and the ready queue based on I/O requests and interrupts.

#include <cstdlib>
#include <string>
#include <vector>
#include <deque>

namespace os_ops {

// Basic OS class for managing processes
class OS {
  public:
    OS(size_t num_of_printers, size_t num_of_disks, size_t num_of_cd_drives) :
        printer_num{num_of_printers}, disk_num{num_of_disks},
        cd_num{num_of_cd_drives}, active_process{nullptr} {
      devices.resize(printer_num + disk_num + num_of_cd_drives);
    }
    ~OS();
    // disable copy/move constructor and copy/move assignment operator
    OS(const OS&) = delete;
    OS(OS&&);
    OS& operator=(const OS&) = delete;
    OS& operator=(OS&&) = delete;

    // getters
    size_t get_printer_num() const {return printer_num;}
    size_t get_disk_num() const {return disk_num;}
    size_t get_cd_num() const {return cd_num;}

    // Remove active process from CPU and add it to device queue of device_type.
    // Request I/O parameters from process.
    void IORequest(char device_type, int device_num);

    // Pop first process from device queue of device_type and add it to the
    // ready queue (I/O request completed).
    void HandleInterrupt(char device_type, int device_num);

    // Add new process to the ready queue.
    void NewProcess();

    // Remove active process from the CPU and free its PCB memory.
    void TerminateActiveProcess();

    // Print contents of device queues or ready queue.
    void Snapshot() const;

  private:
    struct PCB {
      enum state {active, waiting};  // unused
      size_t pid;

      std::string file_name;
      size_t start_mem_loc;
      char op;
      size_t file_size;

      PCB(size_t new_pid) : pid{new_pid}, file_name{""},
                            start_mem_loc{0}, op{'-'}, file_size{0} {}
    };
    //CPU
    PCB* active_process;

    size_t pid_count = 0;
    size_t printer_num, disk_num, cd_num;

    // all devices are stored in on array in order cd/rw->disks->printers
    std::vector<std::deque<PCB*>> devices;
    std::deque<PCB*> ready_queue;

    // Print content of a device queue or the ready queue in 24 line chunks.
    // print_props == true to print process I/O parameters.
    void PrintStatus(const std::deque<PCB*>& device, bool print_props,
                     int& lines_printed) const;
};

// Generate an operating system with numbers of devices taken from user input.
// Returns created OS
OS Sysgen();

}
