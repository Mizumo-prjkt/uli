#include "diskcheck.hpp"
#include <iostream>
#include <string>

int main() {
  std::cout << "--- ULI Disk Checker Engine Test ---" << std::endl;

  // Scan all disks
  std::vector<uli::partitioner::DiskInfo> disks =
      uli::partitioner::DiskCheck::scan_disks();
  std::cout << "Detected " << disks.size() << " valid block devices:\n";
  for (const auto &disk : disks) {
    std::cout << "  - [" << disk.name << "] " << disk.path
              << " | Size: " << disk.size
              << " | Has OS: " << (disk.has_os ? "Yes" : "No")
              << " | Is Empty: " << (disk.is_empty ? "Yes" : "No") << "\n";
  }

  std::cout << "\n--- Fallback Analysis Test ---\n";
  // Target a bogus disk to force the fallback check logic
  std::string safe_target =
      uli::partitioner::DiskCheck::select_target_disk("/dev/nvme99n1");
  std::cout << "\nSelected Installation Target: "
            << (safe_target.empty() ? "NONE_FOUND_FATAL" : safe_target)
            << std::endl;

  return 0;
}
