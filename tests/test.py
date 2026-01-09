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

    # Set up environment - manifest is in build/bin/
    manifest_path = project_root / "build" / "bin" / "ox_openxr.json"
    env = os.environ.copy()
    env["XR_RUNTIME_JSON"] = str(manifest_path)

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
    print(f"Using runtime: {manifest_path}")
    print(f"Running: {test_script}")
    print("-" * 60)

    result = subprocess.run([str(venv_python), str(test_script)], env=env, capture_output=False)

    print("-" * 60)

    if result.returncode == 0:
        print("\n✅ Test successful!")
        return 0
    else:
        print("\n❌ Test failed!")
        return 1


if __name__ == "__main__":
    sys.exit(main())
