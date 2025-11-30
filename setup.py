#!/usr/bin/env python3
"""
expml - ML Experiment Tracking with TUI
"""

from setuptools import setup, find_packages, Extension
from setuptools.command.build_ext import build_ext
import os
import subprocess
import sys
import glob

class BuildCTUI(build_ext):
    """Custom build command to compile C TUI binary"""
    
    def run(self):
        """Compile the C binary"""
        print("=" * 70)
        print("Building C TUI component...")
        print("=" * 70)
        
        # Check for required libraries
        required_libs = ['ncursesw', 'libcjson']
        missing_libs = []
        
        for lib in required_libs:
            if subprocess.call(['pkg-config', '--exists', lib]) != 0:
                missing_libs.append(lib)
        
        if missing_libs:
            print("\n❌ ERROR: Missing required libraries:", ', '.join(missing_libs))
            print("\nPlease install them:")
            print("  Ubuntu/Debian: sudo apt-get install libncursesw5-dev libcjson-dev")
            print("  Fedora/RHEL:   sudo dnf install ncurses-devel cjson-devel")
            print("  macOS:         brew install ncurses cjson")
            sys.exit(1)
        
        # Gather C sources
        c_sources = glob.glob("src/*.c")
        if not c_sources:
            print("❌ ERROR: No C source files found in src/")
            sys.exit(1)
        
        # Output location
        output_dir = os.path.join("expml")
        os.makedirs(output_dir, exist_ok=True)
        output_binary = os.path.join(output_dir, "expml_tui")
        
        # Compile command
        compile_cmd = [
            "gcc",
            "-O2",
            "-Wall",
            "-Wextra",
            "-Isrc",
            "-o", output_binary
        ] + c_sources + [
            "-lncursesw",
            "-lcjson",
            "-lm"
        ]
        
        print(f"Compiling: {' '.join(compile_cmd)}")
        
        try:
            subprocess.check_call(compile_cmd)
            print(f"✅ Successfully compiled to {output_binary}")
        except subprocess.CalledProcessError as e:
            print(f"❌ Compilation failed with error code {e.returncode}")
            sys.exit(1)
        
        # Continue with normal extension building (if any)
        super().run()

# Read long description from README
long_description = ""
if os.path.exists("README.md"):
    with open("README.md", "r", encoding="utf-8") as f:
        long_description = f.read()

# Dummy extension to trigger build_ext
dummy_extension = Extension(
    name="expml._dummy",
    sources=[]
)

setup(
    name="expml",
    version="0.1.0",
    author="Your Name",
    author_email="your.email@example.com",
    description="ML Experiment Tracking with Beautiful TUI",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/yourusername/expml",
    packages=find_packages(),
    include_package_data=True,
    
    # Python dependencies
    install_requires=[
        "psutil>=5.8.0",
    ],
    
    # Include the compiled binary
    package_data={
        "expml": ["expml_tui"],
    },
    
    # Command-line entry point
    entry_points={
        "console_scripts": [
            "expml=expml.cli:main",
        ],
    },
    
    # Custom build for C binary
    ext_modules=[dummy_extension],
    cmdclass={"build_ext": BuildCTUI},
    
    # Metadata
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: C",
    ],
    python_requires=">=3.7",
    
    # Keywords for discoverability
    keywords="machine-learning experiment-tracking tui monitoring",
)