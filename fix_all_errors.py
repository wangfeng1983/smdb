#!/usr/bin/env python3
"""Fix all std::unexpected calls and C++23 features in SMDB"""

import os
import re
import glob

def fix_file(filepath):
    """Fix all issues in a single file"""
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    original_content = content

    # Fix 1: Replace std::unexpected with Result<T>
    content = re.sub(r'return std::unexpected\(', 'return Result<void>(', content)

    # Fix 2: Replace starts_ with/ends_with with rfind/compare (C++20 compatible)
    # s.starts_with(prefix) -> s.rfind(prefix, 0) == 0
    # s.ends_with(suffix) -> s.rfind(suffix) == s.size() - suffix.size()

    # For starts_with
    def replace_starts_with(match):
        s = match.group(1)
        prefix = match.group(2)
        # Remove quotes if string literal
        prefix = prefix.strip('"')
        if prefix == s:  # Same variable, can't easily fix
            return f'({s}.rfind({prefix}, 0) == 0)'
        return f'({s}.rfind("{prefix}", 0) == 0)'

    content = re.sub(r'(\w+)\.starts_with\("([^"]+)"\)', replace_starts_with, content)
    content = re.sub(r'(\w+)\.starts_with\((\w+)\)', r'\1.rfind(\2, 0) == 0', content)

    # For ends_with
    def replace_ends_with(match):
        s = match.group(1)
        suffix = match.group(2)
        suffix = suffix.strip('"')
        return f'({s}.size() >= {len(suffix)} && {s}.compare({s}.size() - {len(suffix)}, {len(suffix)}, "{suffix}") == 0)'

    content = re.sub(r'(\w+)\.ends_with\("([^"]+)"\)', replace_ends_with, content)

    # Only write if changed
    if content != original_content:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Fixed: {filepath}")
        return True
    return False

# Fix all .cpp files
src_dir = 'D:/2026/claude/smdb/src'
fixed_count = 0

for cpp_file in glob.glob(os.path.join(src_dir, '**/*.cpp'), recursive=True):
    if fix_file(cpp_file):
        fixed_count += 1

print(f"\nTotal files fixed: {fixed_count}")
