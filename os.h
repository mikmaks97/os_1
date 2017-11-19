// Maximilian Ioffe
// 8 Oct 2017
// A basic model of an operating system that moves processes between device
// queues and the ready queue based on I/O requests and interrupts.
#ifndef OS_H
#define OS_H

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <deque>
#include "pcb.h"
#include "device.h"

namespace os_ops {

// Basic OS class for managing processes
class OS {
  public:
    OS(int num_of_printers, int num_of_disks, int num_of_cd_drives,
       int time_slice, const std::vector<int>& cyl_nums) :
        printer_num{num_of_printers}, disk_num{num_of_disks},
        cd_num{num_of_cd_drives}, active_process{nullptr},
        time_slice_length{time_slice} {
      for (int i = 0; i < cd_num; i++) {
        devices.push_back(Device::make_device('c'));
      }
      for (int i = 0; i < disk_num; i++) {
        Device* new_device = Device::make_device('d');
        Disk* new_disk = static_cast<Disk*>(new_device);
        new_disk->num_of_cylinders = cyl_nums[i];
        devices.push_back(new_disk);
      }
      for (int i = 0; i < printer_num; i++) {
        devices.push_back(Device::make_device('p'));
      }
    }
    ~OS();

    OS(OS&&);
    // disable copy constructor and copy/move assignment operator
    OS(const OS&) = delete;
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

    // Ask for duration of time slice process was in CPU until system call.
    // Return duration.
    int TimeSliceInterrupt();

    // Add new process to the ready queue.
    void NewProcess();

    // Remove active process from the CPU and push it to the back of the ready
    // queue (round robin).
    void EndOfTimeSlice();

    // Remove active process from the CPU and free its PCB memory.
    void TerminateActiveProcess();

    // Print contents of device queues or ready queue.
    void Snapshot() const;

  private:
    //CPU
    PCB* active_process;
    int time_slice_length;
    float CPU_time_sum = 0;
    int num_of_completed = 0;

    size_t pid_count = 0;
    int printer_num, disk_num, cd_num;

    // all devices are stored in one array in order cd/rw->disks->printers
    std::vector<Device*> devices;
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

#endif
