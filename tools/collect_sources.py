#!/usr/bin/env python3
"""
WAMR Source Collection Script

This script collects the necessary WAMR source files from the official
WAMR repository and copies them to the appropriate locations in this
Arduino library.

Usage:
    python3 collect_sources.py <path_to_wamr_repo>

Example:
    python3 collect_sources.py ../wasm-micro-runtime
"""

import os
import sys
import shutil
import glob
from pathlib import Path

# Configuration: Files to include for minimal interpreter build
SOURCE_PATTERNS = {
    # Core version file
    'core/': [
        'version.h',
        'config.h'
    ],

    # Platform layer (ESP-IDF)
    'core/shared/platform/esp-idf/': [
        '*.c', '*.h'
    ],

    # Platform include
    'core/shared/platform/include/': [
        '*.h'
    ],

    # Platform common
    'core/shared/platform/common/': [
        'posix/*.c', 'posix/*.h',
        'libc-util/*.c', 'libc-util/*.h'
    ],

    # Memory allocators
    'core/shared/mem-alloc/': [
        'mem_alloc.c', 'mem_alloc.h',
        'ems/*.c', 'ems/*.h',
        'tlsf/*.c', 'tlsf/*.h'
    ],

    # Shared utilities
    'core/shared/utils/': [
        '*.c', '*.h',
        'uncommon/*.c', 'uncommon/*.h'
    ],

    # IWASM include (public headers)
    'core/iwasm/include/': [
        '*.h'
    ],

    # IWASM common
    'core/iwasm/common/': [
        '*.c', '*.h',
        'arch/invokeNative_xtensa.S'  # Capital .S for assembly (not .s)
    ],

    # Interpreter
    'core/iwasm/interpreter/': [
        '*.c', '*.h'
    ],

    # AOT (for future use, disabled by default)
    'core/iwasm/aot/': [
        '*.c', '*.h',
        'arch/aot_reloc_xtensa.c'
    ],

    # Compilation (for future use, disabled by default)
    'core/iwasm/compilation/': [
        '*.c', '*.h'
    ],

    # libc-builtin
    'core/iwasm/libraries/libc-builtin/': [
        '*.c', '*.h'
    ],

    # Thread manager (if needed)
    'core/iwasm/libraries/thread-mgr/': [
        '*.c', '*.h'
    ],

    # libc-wasi (minimal, optional)
    'core/iwasm/libraries/libc-wasi/': [
        '*.h'
    ],
}

# Files to explicitly exclude
EXCLUDE_FILES = [
    'wasm_interp_classic.c',  # We use fast interpreter
    'wasm_mini_loader.c',     # We use normal loader
    '_test.c',                # Test files
    '_tests.c',
]

def collect_files(wamr_root, dest_root):
    """Collect WAMR source files to destination directory."""
    wamr_root = Path(wamr_root).resolve()
    dest_root = Path(dest_root).resolve()

    if not wamr_root.exists():
        print(f"Error: WAMR repository not found at {wamr_root}")
        sys.exit(1)

    print(f"Collecting sources from: {wamr_root}")
    print(f"Destination: {dest_root}")
    print()

    copied_files = []
    skipped_files = []

    for source_dir, patterns in SOURCE_PATTERNS.items():
        source_path = wamr_root / source_dir

        if not source_path.exists():
            print(f"Warning: Source directory not found: {source_path}")
            continue

        print(f"Processing: {source_dir}")

        for pattern in patterns:
            # Find matching files (use recursive=True to handle subdirectory patterns)
            matches = glob.glob(str(source_path / pattern), recursive=True)

            for match in matches:
                match_path = Path(match)

                # Skip excluded files
                if any(excl in match_path.name for excl in EXCLUDE_FILES):
                    skipped_files.append(match_path.name)
                    continue

                # Calculate relative path from wamr core directory
                # For files directly in core/, just use the filename
                if source_dir == 'core/':
                    rel_path = Path(match_path.name)
                else:
                    rel_path = match_path.relative_to(wamr_root / 'core')

                # Destination path
                dest_file = dest_root / 'src' / 'wamr' / rel_path

                # Create destination directory
                dest_file.parent.mkdir(parents=True, exist_ok=True)

                # Copy file
                shutil.copy2(match, dest_file)
                copied_files.append(str(rel_path))
                print(f"  Copied: {rel_path}")

    print()
    print(f"Summary:")
    print(f"  Files copied: {len(copied_files)}")
    print(f"  Files skipped: {len(skipped_files)}")

    if skipped_files:
        print(f"  Skipped files: {', '.join(set(skipped_files))}")

    # Write file list to documentation
    file_list_path = dest_root / 'docs' / 'SOURCE_FILES.md'
    with open(file_list_path, 'w') as f:
        f.write(f"# WAMR Source Files\n\n")
        f.write(f"This library includes the following files from WAMR:\n\n")
        f.write(f"**Total files:** {len(copied_files)}\n\n")
        f.write(f"## File List\n\n")
        for file in sorted(copied_files):
            f.write(f"- `{file}`\n")

    print(f"\nFile list written to: {file_list_path}")

    return len(copied_files)

def get_wamr_version(wamr_root):
    """Try to get WAMR version from git or version file."""
    wamr_root = Path(wamr_root)

    # Try git
    git_dir = wamr_root / '.git'
    if git_dir.exists():
        import subprocess
        try:
            result = subprocess.run(
                ['git', 'describe', '--tags', '--always'],
                cwd=wamr_root,
                capture_output=True,
                text=True
            )
            if result.returncode == 0:
                return result.stdout.strip()
        except:
            pass

    # Try version.h
    version_file = wamr_root / 'core' / 'version.h'
    if version_file.exists():
        with open(version_file) as f:
            for line in f:
                if 'WAMR_VERSION' in line:
                    return line.split('"')[1] if '"' in line else 'unknown'

    return 'unknown'

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 collect_sources.py <path_to_wamr_repo>")
        print()
        print("Example:")
        print("  python3 collect_sources.py ../wasm-micro-runtime")
        sys.exit(1)

    wamr_repo = sys.argv[1]
    script_dir = Path(__file__).parent
    repo_root = script_dir.parent

    # Get WAMR version
    version = get_wamr_version(wamr_repo)
    print(f"WAMR Version: {version}\n")

    # Collect sources
    count = collect_files(wamr_repo, repo_root)

    # Write version info
    version_file = repo_root / 'src' / 'wamr' / 'WAMR_VERSION.txt'
    with open(version_file, 'w') as f:
        f.write(f"{version}\n")

    print(f"\nVersion info written to: {version_file}")
    print(f"\n✓ Source collection complete!")
    print(f"\nNext steps:")
    print(f"  1. Review copied files in src/wamr/")
    print(f"  2. Build the library with your PlatformIO/Arduino project")
    print(f"  3. Test with example sketches")

if __name__ == '__main__':
    main()
