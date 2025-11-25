import os
import sys
import subprocess

def main():
    # 1. Find the directory where this script (cli.py) lives
    package_dir = os.path.dirname(os.path.abspath(__file__))
    
    # 2. Look for the binary named 'expml_tui' in the same folder
    binary_path = os.path.join(package_dir, "expml_tui")

    if not os.path.exists(binary_path):
        print(f"Error: expml binary not found at {binary_path}")
        print("Installation might be corrupt.")
        sys.exit(1)

    # 3. Pass all arguments from python to the C binary
    # sys.argv[0] is 'expml', so we pass sys.argv[1:]
    cmd = [binary_path] + sys.argv[1:]
    
    try:
        # Replace the current python process with the C process
        # This is cleaner than subprocess.run as it handles signals better
        os.execv(binary_path, cmd)
    except OSError as e:
        print(f"Error executing binary: {e}")
        sys.exit(1)
