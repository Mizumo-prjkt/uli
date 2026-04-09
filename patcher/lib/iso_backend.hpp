#ifndef ULI_PATCHER_ISO_BACKEND_HPP
#define ULI_PATCHER_ISO_BACKEND_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace uli {
namespace patcher {

// Result of an ISO operation
struct IsoResult {
    bool success = false;
    std::string error;
};

// Abstract interface for ISO manipulation
class IsoBackend {
public:
    virtual ~IsoBackend() = default;

    // Open an existing ISO image for modification
    virtual IsoResult open(const std::string& iso_path) = 0;

    // Inject a file from disk into the ISO filesystem
    virtual IsoResult inject_file(const std::string& disk_path, const std::string& iso_path) = 0;

    // Extract a file from the ISO filesystem to disk
    virtual IsoResult extract_file(const std::string& iso_path, const std::string& disk_path) = 0;

    // Create a directory inside the ISO
    virtual IsoResult mkdir(const std::string& iso_path) = 0;

    // List files at a given ISO path (returns file/dir names)
    virtual std::vector<std::string> ls(const std::string& iso_path) = 0;

    // Check if a path exists inside the ISO
    virtual bool exists(const std::string& iso_path) = 0;

    // Write the modified ISO to an output path
    virtual IsoResult write(const std::string& output_path) = 0;

    // Extract the EFI boot image partition to a local file
    virtual IsoResult extract_boot_image(const std::string& output_disk_path) = 0;

    // Set a custom EFI boot image to be used for the next write
    virtual void set_efi_boot_image(const std::string& disk_path) = 0;

    // Close and clean up
    virtual void close() = 0;

    // Get backend name for diagnostics
    virtual std::string name() const = 0;
};

// Factory: tries xorriso API first, then CLI fallback
std::unique_ptr<IsoBackend> create_iso_backend();

// === Xorriso C API Backend ===
#ifdef ULI_HAS_LIBISOBURN



class XorrisoApiBackend : public IsoBackend {
public:
    XorrisoApiBackend();
    ~XorrisoApiBackend() override;

    IsoResult open(const std::string& iso_path) override;
    IsoResult inject_file(const std::string& disk_path, const std::string& iso_path) override;
    IsoResult extract_file(const std::string& iso_path, const std::string& disk_path) override;
    IsoResult mkdir(const std::string& iso_path) override;
    std::vector<std::string> ls(const std::string& iso_path) override;
    bool exists(const std::string& iso_path) override;
    IsoResult write(const std::string& output_path) override;
    IsoResult extract_boot_image(const std::string& output_disk_path) override;
    void set_efi_boot_image(const std::string& disk_path) override;
    void close() override;
    std::string name() const override { return "xorriso-api"; }

private:
    void* xorriso_ = nullptr;
    bool initialized_ = false;
    std::string custom_efi_img_;
    std::string iso_path_;

    // Helper: run an xorriso interpreter command and check result
    IsoResult run_cmd(const std::string& cmd);
};

#endif // ULI_HAS_LIBISOBURN

// === Xorriso CLI Fallback Backend ===
class XorrisoCmdBackend : public IsoBackend {
public:
    XorrisoCmdBackend();
    ~XorrisoCmdBackend() override;

    IsoResult open(const std::string& iso_path) override;
    IsoResult inject_file(const std::string& disk_path, const std::string& iso_path) override;
    IsoResult extract_file(const std::string& iso_path, const std::string& disk_path) override;
    IsoResult mkdir(const std::string& iso_path) override;
    std::vector<std::string> ls(const std::string& iso_path) override;
    bool exists(const std::string& iso_path) override;
    IsoResult write(const std::string& output_path) override;
    IsoResult extract_boot_image(const std::string& output_disk_path) override;
    void set_efi_boot_image(const std::string& disk_path) override;
    void close() override;
    std::string name() const override { return "xorriso-cli"; }

private:
    std::string iso_path_;
    std::string temp_dir_;
    std::string custom_efi_img_;
    bool opened_ = false;

    // Helper: execute xorriso CLI command, return stdout
    std::pair<int, std::string> exec_xorriso(const std::string& args);
    
    // Check if xorriso binary is available
    static bool is_xorriso_available();
};

} // namespace patcher
} // namespace uli

#endif // ULI_PATCHER_ISO_BACKEND_HPP
