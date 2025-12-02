from setuptools import setup, find_packages, Extension
from setuptools.command.build_ext import build_ext
import os
import subprocess
import sys
import shutil

class BuildCTUI(build_ext):
    def run(self):
        print("=" * 60)
        print("expml v0.1.0: Building via Makefile...")
        print("=" * 60)
        
        # 1. Run Make (compiles to bin/expml)
        try:
            subprocess.check_call(["make", "all"])
        except subprocess.CalledProcessError:
            print("\n❌ Compilation Failed.")
            sys.exit(1)
            
        # 2. Move binary to python/expml/expml_tui
        src_bin = os.path.join("bin", "expml")
        # NOTE: Destination is now inside python/expml/
        dest_bin = os.path.join("python", "expml", "expml_tui")
        
        if not os.path.exists(src_bin):
            print(f"❌ Error: {src_bin} missing.")
            sys.exit(1)
            
        print(f"Installing binary: {src_bin} -> {dest_bin}")
        shutil.copy2(src_bin, dest_bin)
        os.chmod(dest_bin, 0o755)
        print("✅ Build successful.")

setup(
    name="expml",
    version="0.1.0",
    author="Your Name",
    description="ML Experiment Tracking TUI",
    
    # TELL PYTHON WHERE THE CODE IS:
    package_dir={"": "python"}, 
    packages=find_packages(where="python"),
    
    include_package_data=True,
    install_requires=["psutil>=5.8.0"],
    
    # Binary location inside package
    package_data={"expml": ["expml_tui"]},
    
    entry_points={"console_scripts": ["expml=expml.cli:main"]},
    ext_modules=[Extension("expml._dummy", sources=[])],
    cmdclass={"build_ext": BuildCTUI},
)