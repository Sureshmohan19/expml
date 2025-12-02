import threading
import time
import psutil

class SystemMonitor:
    def __init__(self, writer, interval=2.0):
        self.writer = writer
        self.interval = interval
        self.running = False
        self.thread = None
        self.step_ref = lambda: 0 # callback to get current step

    def start(self, step_getter):
        self.step_ref = step_getter
        self.running = True
        self.thread = threading.Thread(target=self._loop, daemon=True)
        self.thread.start()

    def stop(self):
        self.running = False
        if self.thread:
            self.thread.join()

    def _loop(self):
        while self.running:
            try:
                stats = {
                    "system/cpu_percent": psutil.cpu_percent(),
                    "system/ram_percent": psutil.virtual_memory().percent,
                    "system/ram_gb": psutil.virtual_memory().used / (1024**3),
                }
                
                # We construct the entry here
                entry = {
                    "_step": self.step_ref(),
                    "_timestamp": time.time(),
                    **stats
                }
                
                self.writer.log_metrics(entry)
                time.sleep(self.interval)
            except Exception:
                break
