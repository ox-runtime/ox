#!/usr/bin/env python3
"""Test script for ox runtime"""

import os
import sys
import subprocess
import platform
from pathlib import Path


def main():
    # Get the project root directory
    project_root = Path(__file__).parent.parent.absolute()

    print("Testing ox runtime...")
    print()

    # Check environment
    if "XR_RUNTIME_JSON" not in os.environ:
        print("❌ XR_RUNTIME_JSON environment variable is not set.")
        print("Please run this script from the project root directory.")
        return 1

    # Find Python executable in venv
    venv_python = project_root / ".venv" / "Scripts" / "python.exe"
    if not venv_python.exists():
        venv_python = project_root / ".venv" / "bin" / "python"

    if not venv_python.exists():
        print("❌ Python venv not found at .venv/")
        print("Please create a virtual environment first")
        return 1

    # Test script path
    test_script = project_root / "pyopenxr_examples" / "xr_examples" / "runtime_name.py"

    if not test_script.exists():
        print(f"❌ Test script not found: {test_script}. Please checkout https://github.com/cmbruns/pyopenxr_examples")
        return 1

    # Run the test
    print(f"Using runtime: {os.environ.get('XR_RUNTIME_JSON')}")
    print(f"Running: {test_script}")
    print("-" * 60)

    result = subprocess.run([str(venv_python), str(test_script)], capture_output=False)
    print("-" * 60)

    if result.returncode == 0:
        print("\n✅ Test successful!")
        return 0
    else:
        print("\n❌ Test failed!")
        return 1


if __name__ == "__main__":
    sys.exit(main())
