import sys
import os
import argparse

# Ensure the local lib/ is in path if necessary
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from lib.tui import MemLeakApp
from lib.hook import StraceApp

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Universal Linux Installer Codetest Tracing Suite")
    parser.add_argument("--mode", choices=["memleak", "trace"], default="trace", help="Diagnostic mode to run")
    args = parser.parse_args()

    # Locate the compiled uli_installer binary
    base_dir = os.path.dirname(os.path.abspath(__file__))
    binary_path = os.path.abspath(os.path.join(base_dir, "../../../../build/uli_installer"))
    
    if not os.path.exists(binary_path):
        print(f"Error: binary '{binary_path}' not found.")
        print("Please compile uli_installer in the build directory first.")
        sys.exit(1)
        
    if args.mode == "memleak":
        app = MemLeakApp(binary_path=binary_path)
    else:
        app = StraceApp(binary_path=binary_path)
        
    app.run()
