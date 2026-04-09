import asyncio
import re
from typing import Callable, Any

class StraceRunner:
    """Spawns a target binary wrapped in strace and streams system calls."""
    
    # Matches typical strace output like: 
    # openat(AT_FDCWD, "/etc/os-release", O_RDONLY|O_CLOEXEC) = 3
    # execve("/usr/bin/pacman", ["pacman", "-S"], 0x7ffd...) = 0
    SYSCALL_REGEX = re.compile(r'^([a-zA-Z0-9_]+)\((.*)\)\s+=\s+(.*)$')

    def __init__(self, binary_path: str):
        self.binary_path = binary_path
        self.process = None

    async def run(self, sys_callback: Callable[[str, str, str], Any], stdout_callback: Callable[[str], Any]):
        """
        Runs the strace wrapper.
        sys_callback gets called with (syscall_name, arguments, result).
        stdout_callback gets called with raw stdout lines from the binary.
        """
        cmd = ["strace", "-e", "trace=file,process,network", "-f", self.binary_path]
        
        try:
            # strace outputs trace data to stderr, while stdout remains the program's output
            self.process = await asyncio.create_subprocess_exec(
                *cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
            )
        except Exception as e:
            stdout_callback(f"Failed to start strace: {e}")
            return

        asyncio.create_task(self._read_stderr(sys_callback))
        await self._read_stdout(stdout_callback)

    async def _read_stderr(self, callback: Callable[[str, str, str], Any]):
        while True:
            line = await self.process.stderr.readline()
            if not line:
                break
            line_str = line.decode('utf-8', errors='replace').strip()
            # Try parsing the syscall
            match = self.SYSCALL_REGEX.match(line_str)
            if match:
                syscall, args, result = match.groups()
                callback(syscall, args, result)
            else:
                # Some lines might be signals or other info
                if line_str and not line_str.startswith("strace:"):
                    callback("INFO", line_str, "")

    async def _read_stdout(self, callback: Callable[[str], Any]):
        while True:
            line = await self.process.stdout.readline()
            if not line:
                break
            callback(line.decode('utf-8', errors='replace').rstrip())
        
        await self.process.wait()
        callback(f"[Process Exit Code: {self.process.returncode}]")
