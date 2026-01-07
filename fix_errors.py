#!/usr/bin/env python3
"""Fix std::unexpected calls in mem_manager.cpp"""

import re

def fix_unexpected_calls(content):
    """Replace std::unexpected with proper Result constructor"""

    # Pattern to match std::unexpected calls
    pattern = r'return std::unexpected\(([^)]+)\)'

    def replace_func(match):
        error_msg = match.group(1)

        # Look backwards to find the return type
        lines_before = content[:match.start()].split('\n')

        # Find the function signature
        for line in reversed(lines_before):
            func_match = re.search(r'Result<(\w+|\w*\*|\s*&|\s*&&)>\s*\w+\s*\(', line)
            if func_match:
                return_type = func_match.group(1).strip()
                return f"return Result<{return_type}>({error_msg})"

        # Default fallback
        return f"return Result<void>({error_msg})"

    # Apply replacement
    fixed_content = re.sub(pattern, replace_func, content)
    return fixed_content

# Read the file
with open('D:/2026/claude/smdb/src/storage/mem_manager.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Fix it
fixed_content = fix_unexpected_calls(content)

# Write back
with open('D:/2026/claude/smdb/src/storage/mem_manager.cpp', 'w', encoding='utf-8') as f:
    f.write(fixed_content)

print("Fixed std::unexpected calls in mem_manager.cpp")
