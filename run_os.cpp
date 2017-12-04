// Maximilian Ioffe
// 8 Oct 2017
// Main file for running a basic OS

#include "os.h"
#include <iostream>
using namespace std;
using namespace os_ops;

int main() {
  OS os = Sysgen();

  while (true) {
    cout << "Waiting for input: ";
    std::string input;
    bool invalid = false;

    // ask for input until input is valid
    do {
      invalid = false;
      cin >> input;
      if (input.size() == 1) {
        if (input[0] == 'A') os.NewProcess();
        else if (input[0] == 't') os.TerminateActiveProcess();
        else if (input[0] == 'S') os.Snapshot();
        else if (input[0] == 'T') os.EndOfTimeSlice();
        else invalid = true;
      }
      // all input of length 2 is upper/lowercase letter followed by number
      else if (input.size() > 1) {
        if (isupper(input[0])) {
          if (input[0] == 'K') {
            string proc_num_str = input.substr(1, input.size()-1);
            char *p;
            int proc_num = strtoul(proc_num_str.c_str(), &p, 10);
            if (!*p) os.Kill(proc_num);
            else invalid = true;
          }
          else {
            string device_num_str = input.substr(1, input.size()-1);
            char *p;
            int device_num = strtoul(device_num_str.c_str(), &p, 10);
            if (!*p) {
              char device_type = input[0];
              if (device_num == 0 ||
                  (device_type != 'P' && device_type != 'D' && device_type != 'C') ||
                  (device_type == 'P' && device_num > os.get_printer_num()) ||
                  (device_type == 'D' && device_num > os.get_disk_num()) ||
                  (device_type == 'C' && device_num > os.get_cd_num()))
                invalid = true;
              else os.HandleInterrupt(device_type, device_num);
            }
            else invalid = true;
          }
        }
        // cpu must have active process for an i/o request
        else if (islower(input[0])) {
          string device_num_str = input.substr(1, input.size()-1);
          char *p;
          int device_num = strtoul(device_num_str.c_str(), &p, 10);
          if (!*p) {
            char device_type = input[0];
            if (device_num == 0 ||
                (device_type != 'p' && device_type != 'd' && device_type != 'c') ||
                (device_type == 'p' && device_num > os.get_printer_num()) ||
                (device_type == 'd' && device_num > os.get_disk_num()) ||
                (device_type == 'c' && device_num > os.get_cd_num()))
              invalid = true;
            else os.IORequest(device_type, device_num);
          }
          else invalid = true;
        }
        else invalid = true;
      }

      if (invalid) {
        cout << "Invalid input. Try again: ";
      }
    } while (invalid);
  }
  return 0;
}
