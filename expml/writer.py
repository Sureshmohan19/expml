import json
import os
import time

class RunWriter:
    def __init__(self, run_dir):
        self.run_dir = run_dir
        self.metrics_path = os.path.join(run_dir, "metrics.jsonl")
        self.summary_path = os.path.join(run_dir, "summary.json")
        self.summary_cache = {}

    def write_config(self, config):
        path = os.path.join(self.run_dir, "config.json")
        with open(path, "w") as f:
            json.dump(config, f, indent=2)

    def write_metadata(self, metadata):
        path = os.path.join(self.run_dir, "metadata.json")
        with open(path, "w") as f:
            json.dump(metadata, f, indent=2)

    def log_metrics(self, data):
        # atomic append
        with open(self.metrics_path, "a") as f:
            f.write(json.dumps(data) + "\n")

    def update_summary(self, data):
        self.summary_cache.update(data)
        # Atomic write (rewrite file)
        with open(self.summary_path, "w") as f:
            json.dump(self.summary_cache, f, indent=2)
