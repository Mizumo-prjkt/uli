import asyncio
import traceback
from textual.app import App, ComposeResult
from textual.widgets import Header, Footer, Static, Log, DataTable
from rich.text import Text

from .tracing import StraceRunner

class StraceApp(App):
    CSS = """
    Screen {
        layout: vertical;
    }
    #syslog {
        height: 60%;
        border: solid yellow;
    }
    #stdoutlog {
        height: 40%;
        border: solid blue;
    }
    """
    
    BINDINGS = [("q", "quit", "Quit tracer")]

    def __init__(self, binary_path: str):
        super().__init__()
        self.binary_path = binary_path
        self.runner = StraceRunner(binary_path)

    def compose(self) -> ComposeResult:
        yield Header(show_clock=True)
        yield DataTable(id="syslog")
        yield Log(id="stdoutlog")
        yield Footer()

    async def on_mount(self) -> None:
        self.syslog = self.query_one("#syslog", DataTable)
        self.syslog.add_columns("Syscall", "Arguments", "Result")
        self.stdoutlog = self.query_one("#stdoutlog", Log)
        
        self.run_worker(self.start_tracing(), exclusive=True)

    async def start_tracing(self):
        try:
            await self.runner.run(self.on_syscall, self.on_stdout)
        except Exception as e:
            self.call_from_thread(self.stdoutlog.write, f"Error launching tracer: {e}")
            self.call_from_thread(self.stdoutlog.write, traceback.format_exc())

    def on_syscall(self, syscall: str, args: str, result: str):
        # Apply gentle formatting
        if syscall in ["execve", "clone"]:
            style = "bold red"
        elif syscall in ["openat", "open", "read"]:
            style = "cyan"
        else:
            style = "white"

        def _add_row():
            # keep table from infinite scrolling and lagging
            if self.syslog.row_count > 1000:
                self.syslog.clear()
            self.syslog.add_row(
                Text(syscall, style=style),
                Text(args[:100] + "..." if len(args) > 100 else args, style="dim"),
                Text(result, style="green")
            )
            self.syslog.scroll_end(animate=False)
            
        self.call_from_thread(_add_row)

    def on_stdout(self, line: str):
        self.call_from_thread(self.stdoutlog.write, line)
