# Tracing for Universal Linux Installer

A tool that checks for possible issues on the universal linux installer, written in Python, using `textual` to provide a Terminal UI (TUI).

## Transparent Runtime Tracing (Default)

This tracer uses the native Linux `strace` subsystem to transparently intercept and catalog the system and file calls made by `uli_installer` during runtime. Unlike memory leak tracing, this method does not require internal flag hooks (`--test-memleak`) and tests the raw binary as-is.

### Usage
Run the main tracer script:
```bash
python3 main.py --mode trace
```

The TUI displays a live interactive DataTable tracking executing calls such as file accesses and executable child spawns.

## Memory Leak Tracer

This tracer hooks into the `uli_installer` binary by running it with the special `--test-memleak` flag, which places the binary into a continuous translation loop to stress test the memory allocations.

### Requirements
Ensure you have the virtual environment activated and requirements installed:
```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

### Usage
Run the main tracer script:
```bash
python3 main.py --mode memleak
```

The TUI will launch and display the realtime memory information (Resident Set Size) of the installer process. If memory continues to grow significantly (e.g., > 50MB anomaly), it will flag a `POTENTIAL MEMORY LEAK DETECTED` warning in red.
