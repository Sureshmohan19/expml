import os
import time
import uuid
from .writer import RunWriter
from .metadata import get_system_info
from .monitor import SystemMonitor

# Global State (Hidden from user)
_CURRENT_RUN = None

class ActiveRun:
    def __init__(self, config=None, name=None):
        self.id = str(uuid.uuid4())[:8]
        self.name = name or f"run-{self.id}"
        self.step = 0
        self.start_time = time.time()
        
        # Setup Directory
        self.root_dir = "expml_runs" # Default
        self.run_dir = os.path.join(self.root_dir, self.name)
        os.makedirs(self.run_dir, exist_ok=True)
        
        # Update Symlink
        symlink = os.path.join(self.root_dir, "latest-run")
        if os.path.lexists(symlink):
            os.remove(symlink)
        os.symlink(self.name, symlink)

        # Initialize Components
        self.writer = RunWriter(self.run_dir)
        self.monitor = SystemMonitor(self.writer)

        # Write Static Files
        self.writer.write_config(config or {})
        
        meta = get_system_info()
        meta["id"] = self.id
        meta["name"] = self.name
        self.writer.write_metadata(meta)

        # Start Monitor
        self.monitor.start(lambda: self.step)
        
        print(f" expml: Run {self.name} initialized")

    def log(self, metrics):
        self.step += 1
        now = time.time()
        
        entry = {
            "_step": self.step,
            "_timestamp": now,
            "_runtime": now - self.start_time,
            **metrics
        }
        
        self.writer.log_metrics(entry)
        
        # Update summary with latest values + status
        summary_update = metrics.copy()
        summary_update.update({
            "status": "RUNNING",
            "_step": self.step,
            "_runtime": now - self.start_time
        })
        self.writer.update_summary(summary_update)

    def finish(self):
        self.monitor.stop()
        
        self.writer.update_summary({
            "status": "FINISHED",
            "_runtime": time.time() - self.start_time
        })
        print(f" expml: Run {self.name} finished")

# --- Public API Functions ---

def init(config=None, name=None, project=None):
    global _CURRENT_RUN
    if _CURRENT_RUN:
        print("Warning: Run already active. Finishing previous run.")
        finish()
    _CURRENT_RUN = ActiveRun(config=config, name=name)

def log(metrics):
    if _CURRENT_RUN:
        _CURRENT_RUN.log(metrics)

def finish():
    global _CURRENT_RUN
    if _CURRENT_RUN:
        _CURRENT_RUN.finish()
        _CURRENT_RUN = None
