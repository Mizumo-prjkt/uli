import os

def get_process_memory(pid: int) -> int:
    """Returns the RSS memory of the process in bytes by reading /proc/[pid]/statm."""
    try:
        with open(f"/proc/{pid}/statm", "r") as f:
            data = f.read().split()
            # statm second field is resident set size (RSS) in pages
            rss_pages = int(data[1])
            page_size = os.sysconf("SC_PAGE_SIZE")
            return rss_pages * page_size
    except (FileNotFoundError, ProcessLookupError, IndexError):
        return -1
