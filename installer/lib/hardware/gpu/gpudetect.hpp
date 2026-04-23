// We detect GPU via LSPCI because early iterations of this Codebase in dev
// (post-ULI, but pre-Haruka) has different GPU ID tag, especially for OEMs like
// Lenovo where Kaby Lake IGPU on their M710q are tagged in uint16 as 0x17aa,
// but official ID is 0x5912.
//
// However, we can still keep the Vendor ID as it is, since it's not OEM
// specific. So, if we can't properly determine the GPU ID, we can fallback
// to the Vendor ID, as a redundancy.

#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
