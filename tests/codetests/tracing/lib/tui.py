import asyncio
from textual.app import App, ComposeResult
from textual.widgets import Header, Footer, Static, Log
import traceback

from .memsniff import get_process_memory

class MemLeakApp(App):
    CSS = """
    Screen {
        layout: vertical;
    }
    #stats {
        height: 6;
        dock: top;
        border: solid green;
        padding: 1;
        background: $boost;
    }
    Log {
        border: solid blue;
    }
    """
    
    BINDINGS = [("q", "quit", "Quit tracer")]

    def __init__(self, binary_path: str):
        super().__init__()
        self.binary_path = binary_path
        self.process = None

    def compose(self) -> ComposeResult:
        yield Header(show_clock=True)
        yield Static("Initializing tracer...", id="stats")
        yield Log(id="log")
        yield Footer()
        
    async def on_mount(self) -> None:
        self.log_widget = self.query_one(Log)
        self.stats_widget = self.query_one("#stats", Static)
        
        # Start background task to run process
        self.run_worker(self.monitor_process(), exclusive=True)

    async def monitor_process(self):
        try:
            self.process = await asyncio.create_subprocess_exec(
                self.binary_path, "--test-memleak",
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.STDOUT,
                # Create a new session, so we can run cleanly
            )
        except Exception as e:
            self.log_widget.write(f"Error launching binary: {e}")
            self.log_widget.write(traceback.format_exc())
            return

        pid = self.process.pid
        self.log_widget.write(f"Launched {self.binary_path} with PID: {pid}")
        
        # Concurrently read stdout while we monitor memory
        asyncio.create_task(self.read_stdout())
        
        max_mem = 0
        start_mem = -1
        
        while self.process.returncode is None:
            mem = get_process_memory(pid)
            if mem != -1:
                if start_mem == -1:
                    start_mem = mem
                
                max_mem = max(max_mem, mem)
                diff = mem - start_mem
                
                # Check for > 50MB anomaly during translation logic
                leak_warning = "[red]⚠️ POTENTIAL MEMORY LEAK DETECTED[/red]" if diff > (50 * 1024 * 1024) else "[green]OK[/green]"
                
                stats_text = (
                    f"PID: {pid} | Target: {self.binary_path}\n"
                    f"Current RSS: {mem / 1024 / 1024:.2f} MB\n"
                    f"Peak RSS: {max_mem / 1024 / 1024:.2f} MB\n"
                    f"Growth: {diff / 1024 / 1024:.2f} MB   {leak_warning}"
                )
                self.call_from_thread(self.stats_widget.update, stats_text)
            
            await asyncio.sleep(0.5)
            
        self.call_from_thread(self.stats_widget.update, "Process exited.")

    async def read_stdout(self):
        while True:
            line = await self.process.stdout.readline()
            if not line:
                break
            # the log widget might be called from an async thread context
            self.call_from_thread(self.log_widget.write, line.decode('utf-8', errors='replace').rstrip())
        self.call_from_thread(self.log_widget.write, "[Process terminated normally]")
