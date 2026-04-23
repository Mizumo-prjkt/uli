#pragma once
#include <atomic>
#include <cerrno>
#include <fcntl.h>
#include <fstream>
#include <linux/fs.h>
#include <string>
#include <sys/ioctl.h>

class StorageDetect {
public:
  StorageDetect() = default;

  // Check if there's sdX nor nvmeX in the /proc/partitions
  bool sdX_drive_exists();
  bool nvmeX_drive_exists();
};
