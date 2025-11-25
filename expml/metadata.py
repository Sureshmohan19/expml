import platform
import socket
import sys
import os
import psutil
import getpass

def get_system_info():
    """Captures static environment metadata."""
    return {
        "user": getpass.getuser(),
        "host": socket.gethostname(),
        "os": platform.platform(),
        "python": platform.python_version(),
        "cpu_count": psutil.cpu_count(),
        "gpu_count": 0, # Placeholder: Add pynvml logic here later
        "gpu_name": "N/A",
        "disk_total": f"{psutil.disk_usage('/').total / (1024**3):.1f}GB",
        "ram_total": f"{psutil.virtual_memory().total / (1024**3):.1f}GB",
        "command": " ".join(sys.argv)
    }
