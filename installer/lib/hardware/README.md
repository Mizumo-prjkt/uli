# Hardware Analyzer for HarukaInstaller

This wrapper is for HarukaInstaller to analyze hardware.

`hwdetect.cpp` is the core of all components for the detection.
`hwdetect.h` is the header file for the core.

Rest of the directories are for each detection type.

Checks for the following

## DRM

Checks what Graphics Card(s) does the user have in their system

## Storage

Checks about what storage device(s) the system has

## CPU

Checks about the CPU of the system, for Microcode application to the kernel

## Network Interface

Checks about the network interface(s) of the system

