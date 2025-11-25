from setuptools import setup, find_packages
from setuptools.command.build_py import build_py
import os
import subprocess
import glob

class CustomBuild(build_py):
    def run(self):
        # 1. Compile the C Binary
        print("🔨 Building C TUI binary...")
        
        # List all C sources
        c_sources = glob.glob("src/*.c")
        output_binary = os.path.join("expml", "expml_tui")
        
        # Compiler arguments
        # Note: We assume the user has gcc, ncurses, and cjson installed.
        # In a fully production app, we would bundle cjson.c source code to avoid external deps.
        compile_cmd = [
            "gcc",
            "-O2", 
            "-Wall", 
            "-Isrc",
            "-o", output_binary
        ] + c_sources + [
            "-lncursesw", 
            "-lcjson", 
            "-lm"
        ]
        
        try:
            subprocess.check_call(compile_cmd)
            print(f"✅ Compiled to {output_binary}")
        except subprocess.CalledProcessError as e:
            print("❌ Compilation failed. Ensure gcc, libncursesw5-dev, and libcjson-dev are installed.")
            raise e

        # 2. Run the standard Python build process
        # This will copy the 'expml' folder (including our new binary) to the build dir
        super().run()

setup(
    name="expml",
    version="0.0.1",
    packages=find_packages(),
    include_package_data=True,
    install_requires=[
        "psutil",
    ],
    # This tells setuptools to include non-python files found in the package (like our binary)
    package_data={
        "expml": ["expml_tui"],
    },
    entry_points={
        'console_scripts': [
            'expml=expml.cli:main',
        ],
    },
    cmdclass={
        'build_py': CustomBuild,
    },
)
