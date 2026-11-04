#!/bin/bash

# Use the first argument as target dir, or default to the script's directory
if [ -n "$1" ]; then
    TARGET_DIR="$1"
else
    TARGET_DIR="$(dirname "$(realpath "$0")")"
fi

printf 'TARGET_DIR: %s\n' "$TARGET_DIR"

# Define the output file
OUTPUT_FILE="$TARGET_DIR/build_info.h"

# Create directory if it doesn't exist
mkdir -p "$(dirname "$OUTPUT_FILE")"

# Append one complete line without interpreting backslash escapes.
append_line() {
    printf '%s\n' "$1" >> "$OUTPUT_FILE"
}

# Start with a fresh file.
: > "$OUTPUT_FILE"
append_line "/* Auto-generated build information */"
append_line "#ifndef __BUILD_INFO_H__"
append_line "#define __BUILD_INFO_H__"
append_line ""
append_line "#include <stdio.h>"
append_line ""

# Add build timestamp
append_line "#define BUILD_TIMESTAMP \"$(date '+%Y-%m-%d %H:%M:%S')\""
append_line "#define BUILD_SHORT_TS \"$(date '+%y%m%d%H')\""

# Find git repository root directory
GIT_ROOT_DIR=$(git rev-parse --show-toplevel 2>/dev/null)

# Escape text before placing it in a generated C string literal.
c_escape() {
    local value="$1"
    value=${value//\\/\\\\}
    value=${value//\"/\\\"}
    printf '%s' "$value"
}

GIT_SUBMODULE_COUNT=0
GIT_SUBMODULE_INFO=""

# Add Git information if available
if [ -n "$GIT_ROOT_DIR" ] && [ -d "$GIT_ROOT_DIR/.git" ]; then
    # We're in a git repository, use the root directory for git commands
    append_line "#define GIT_HASH \"$(git rev-parse --short HEAD)\""
    append_line "#define GIT_COMMIT_HASH \"$(git rev-parse HEAD)\""
    append_line "#define GIT_BRANCH \"$(git rev-parse --abbrev-ref HEAD)\""
    
    # Get tag if available
    GIT_TAG=$(git describe --tags --exact-match 2>/dev/null || printf '')
    if [ -n "$GIT_TAG" ]; then
        append_line "#define GIT_TAG \"$GIT_TAG\""
    else
        append_line "#define GIT_TAG \"No Tag\""
    fi
    
    # Check for dirty working tree
    if [ -n "$(git status --porcelain)" ]; then
        append_line "#define GIT_DIRTY 1"
    else
        append_line "#define GIT_DIRTY 0"
    fi
    
    # Get repo URL - check for any remote, not just origin
    GIT_REPO_URL=$(git remote -v | grep fetch | head -1 | awk '{print $2}' 2>/dev/null || printf 'Unknown')
    append_line "#define GIT_REPO_URL \"$GIT_REPO_URL\""

    # Record configured submodules, including ones not initialized locally.
    if [ -f "$GIT_ROOT_DIR/.gitmodules" ]; then
        while read -r config_key submodule_path; do
            [ -n "$submodule_path" ] || continue

            submodule_key=${config_key%.path}
            submodule_url=$(git config -f "$GIT_ROOT_DIR/.gitmodules" \
                --get "$submodule_key.url" 2>/dev/null || printf 'Unknown')
            submodule_dir="$GIT_ROOT_DIR/$submodule_path"

            if git -C "$submodule_dir" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
                submodule_hash=$(git -C "$submodule_dir" rev-parse --short HEAD)
                submodule_branch=$(git -C "$submodule_dir" symbolic-ref --quiet --short HEAD 2>/dev/null || printf 'detached')
                submodule_tag=$(git -C "$submodule_dir" describe --tags --exact-match 2>/dev/null || printf 'No Tag')
                if [ -n "$(git -C "$submodule_dir" status --porcelain)" ]; then
                    submodule_dirty=1
                else
                    submodule_dirty=0
                fi
            else
                submodule_hash="not initialized"
                submodule_branch="unknown"
                submodule_tag="No Tag"
                submodule_dirty=0
            fi

            GIT_SUBMODULE_INFO+="  $(c_escape "$submodule_path"): commit=$(c_escape "$submodule_hash"), branch=$(c_escape "$submodule_branch"), tag=$(c_escape "$submodule_tag"), dirty=$submodule_dirty, url=$(c_escape "$submodule_url")\\r\\n"
            GIT_SUBMODULE_COUNT=$((GIT_SUBMODULE_COUNT + 1))
        done < <(git config -f "$GIT_ROOT_DIR/.gitmodules" \
            --get-regexp '^submodule\..*\.path$' 2>/dev/null)
    fi
else
    append_line "#define GIT_HASH \"\""
    append_line "#define GIT_COMMIT_HASH \"\""
    append_line "#define GIT_BRANCH \"\""
    append_line "#define GIT_TAG \"No Tag\""
    append_line "#define GIT_DIRTY 0"
    append_line "#define GIT_REPO_URL \"Unknown\""
fi

if [ "$GIT_SUBMODULE_COUNT" -eq 0 ]; then
    GIT_SUBMODULE_INFO="  None\\r\\n"
fi
append_line "#define GIT_SUBMODULE_COUNT $GIT_SUBMODULE_COUNT"
printf '#define GIT_SUBMODULE_INFO "%s"\n' "$GIT_SUBMODULE_INFO" >> "$OUTPUT_FILE"

# Add build environment info
append_line "#define BUILD_HOST \"$(hostname)\""
append_line "#define BUILD_USER \"$(whoami)\""
append_line "#define BUILD_OS \"$(uname -sr)\""

# Add const arrays for easy binary inclusion
append_line ""
append_line "// Const arrays for binary inclusion in firmware"
append_line "#ifdef BUILD_INFO_IMPLEMENTATION"
append_line "// These are defined only once in the file that defines BUILD_INFO_IMPLEMENTATION"
append_line "const char git_hash[] = GIT_HASH;"
append_line "const char git_branch[] = GIT_BRANCH;"
append_line "const char git_tag[] = GIT_TAG;"
append_line "const char git_submodules[] = GIT_SUBMODULE_INFO;"
append_line "const char build_ts[] = BUILD_TIMESTAMP;"
append_line "const char build_short_ts[] = BUILD_SHORT_TS;"
append_line "#else"
append_line "// In all other files, just declare them as extern"
append_line "extern const char git_hash[];"
append_line "extern const char git_branch[];"
append_line "extern const char git_tag[];"
append_line "extern const char git_submodules[];"
append_line "extern const char build_ts[];"
append_line "extern const char build_short_ts[];"
append_line "#endif // BUILD_INFO_IMPLEMENTATION"

# Add the declaration
append_line ""
append_line "// Helper function to print build information"
append_line "void print_build_info(void);"

# Add the definition with inline
append_line ""
append_line "// Implementation of the print function"
append_line "#ifdef BUILD_INFO_IMPLEMENTATION"
append_line "void print_build_info(void) {"
append_line $'\tprintf("\\r\\n");'
append_line $'\tprintf("Build Timestamp: %s\\r\\n", BUILD_TIMESTAMP);'
append_line $'\tprintf("Build short Timestamp: %s\\r\\n", BUILD_SHORT_TS);'
append_line $'\tprintf("Git Repository URL: %s\\r\\n", GIT_REPO_URL);'
append_line $'\tprintf("Git Branch: %s\\r\\n", GIT_BRANCH);'
append_line $'\tprintf("Git Tag: %s\\r\\n", GIT_TAG);'
append_line $'\tprintf("Git Commit Hash: %s\\r\\n", GIT_COMMIT_HASH);'
append_line $'\tprintf("Git Hash: %s\\r\\n", GIT_HASH);'
append_line $'\tprintf("Repository Dirty: %d\\r\\n", GIT_DIRTY);'
append_line $'\tprintf("Git Submodules (%d):\\r\\n%s", GIT_SUBMODULE_COUNT, GIT_SUBMODULE_INFO);'
append_line $'\tprintf("Build Host: %s\\r\\n", BUILD_HOST);'
append_line $'\tprintf("Build User: %s\\r\\n", BUILD_USER);'
append_line $'\tprintf("Build OS: %s\\r\\n", BUILD_OS);'
append_line $'\tprintf("\\r\\n");'
append_line "}"
append_line "#endif // BUILD_INFO_IMPLEMENTATION"

# Close the header guard
append_line "#endif /* __BUILD_INFO_H__ */"

printf 'Build info generated at %s\n' "$OUTPUT_FILE"
