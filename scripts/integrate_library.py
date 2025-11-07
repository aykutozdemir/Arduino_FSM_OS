#!/usr/bin/env python3
"""
Script to integrate an external library into FsmOS framework
Usage: python3 integrate_library.py <github_url> [library_name]
"""

import sys
import os
import subprocess
from pathlib import Path

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 integrate_library.py <github_url> [library_name]")
        print("Example: python3 integrate_library.py https://github.com/user/library.git MyLibrary")
        sys.exit(1)

    github_url = sys.argv[1]
    library_name = sys.argv[2] if len(sys.argv) > 2 else None

    # Extract library name from URL if not provided
    if not library_name:
        library_name = os.path.basename(github_url).replace('.git', '')

    script_dir = Path(__file__).parent.absolute()
    libs_dir = script_dir.parent / 'libs'
    lib_dir = libs_dir / library_name

    print(f"Integrating library: {library_name}")
    print(f"From: {github_url}")
    print(f"To: {lib_dir}")

    # Create libs directory if it doesn't exist
    libs_dir.mkdir(parents=True, exist_ok=True)

    # Clone or update the repository
    if lib_dir.exists():
        print(f"Directory {lib_dir} already exists. Updating...")
        try:
            subprocess.run(['git', 'pull'], cwd=lib_dir, check=True)
        except subprocess.CalledProcessError as e:
            print(f"Warning: Failed to update repository: {e}")
    else:
        print("Cloning repository...")
        try:
            subprocess.run(['git', 'clone', github_url, str(lib_dir)], check=True)
        except subprocess.CalledProcessError as e:
            print(f"Error: Failed to clone repository: {e}")
            sys.exit(1)

    print("\nLibrary integrated successfully!")
    print(f"Location: {lib_dir}")
    print("\nNext steps:")
    print("1. Review the library structure")
    print("2. Check for compatibility with FsmOS")
    print("3. Update library.properties if needed")
    print("4. Test integration")

if __name__ == '__main__':
    main()

