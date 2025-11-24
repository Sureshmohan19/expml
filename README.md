# expml

terminal-based ML experiment tracker 🎧.

## Overview

expml is a lightweight, terminal focused experiment tracker designed for machine learning workflows. 
It gives you Weights and Biases style experiment logging but inside your terminal without any cloud storage. 
It is ideal for researchers, students, and developers who want simple and fast experiment tracking with minimal overhead.

## Features

* **TUI based interface** to browse experiments, runs, metrics, and logs.
* **Local first storage** for experiment metadata and metrics.
* **Simple Python API** for logging metrics, parameters, and notes.
* **Lightweight and dependency minimal**. Suitable for small projects and research prototypes.
* **Reproducibility support** by automatically tracking parameters and timestamps.

## Installation

```bash
pip install expml
```

## Quick Start

### 1. Initialize and Log metrics of a new experiment

```python
import expml

expml.init()
exp.log_param("lr", 0.001)
exp.log_param("batch_size", 64)
expml.end()
```

### 3. View runs in the terminal

```bash
expml run
```

This opens an interactive TUI where you can view experiments, runs, and metrics.

## Project Goals

* Provide a minimal alternative to W&B for terminal users.
* Keep the project open source, simple, and extensible.
* htop level details with simplicity

## Roadmap

* CLI commands for managing experiments.
* Support for exporting run data to JSON or CSV.
* Improved visualizations inside the TUI.
* Plugin system for custom renderers.

## Contributing

Contributions are welcome. Feel free to open issues or submit pull requests for bugs, improvements, or documentation.

## License

This project is released under the MIT License.
