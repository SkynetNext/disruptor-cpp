#!/bin/bash
# Local script to run clang-tidy on the codebase
# Usage: ./scripts/run_clang_tidy.sh [options]
#
# Options:
#   --fix          Apply automatic fixes where possible
#   --quiet        Suppress non-error output
#   --verbose      Show detailed output
#   --files FILE   Only check specific files (space-separated)
#   --help         Show this help message

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default options
FIX_MODE=""
QUIET=""
VERBOSE=""
FILES_SPECIFIED=""
BUILD_DIR="build"

# Parse arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --fix)
      FIX_MODE="--fix"
      shift
      ;;
    --quiet)
      QUIET="--quiet"
      shift
      ;;
    --verbose)
      VERBOSE="--verbose"
      shift
      ;;
    --files)
      FILES_SPECIFIED="$2"
      shift 2
      ;;
    --build-dir)
      BUILD_DIR="$2"
      shift 2
      ;;
    --help)
      echo "Usage: $0 [options]"
      echo ""
      echo "Options:"
      echo "  --fix          Apply automatic fixes where possible"
      echo "  --quiet        Suppress non-error output"
      echo "  --verbose      Show detailed output"
      echo "  --files FILE   Only check specific files (space-separated)"
      echo "  --build-dir DIR Build directory (default: build)"
      echo "  --help         Show this help message"
      exit 0
      ;;
    *)
      echo -e "${RED}Unknown option: $1${NC}"
      echo "Use --help for usage information"
      exit 1
      ;;
  esac
done

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

echo -e "${GREEN}=== Running clang-tidy locally ===${NC}"
echo ""

# Check if clang-tidy is available
if ! command -v clang-tidy &> /dev/null; then
  echo -e "${RED}Error: clang-tidy not found in PATH${NC}"
  echo "Please install clang-tidy:"
  echo "  - Ubuntu/Debian: sudo apt-get install clang-tidy"
  echo "  - macOS: brew install llvm"
  echo "  - Windows: Install LLVM from https://llvm.org/builds/"
  exit 1
fi

CLANG_TIDY_VERSION=$(clang-tidy --version | head -n1)
echo -e "${GREEN}Found: $CLANG_TIDY_VERSION${NC}"
echo ""

# Check if .clang-tidy config exists
if [ ! -f ".clang-tidy" ]; then
  echo -e "${YELLOW}Warning: .clang-tidy config file not found${NC}"
  echo "clang-tidy will use default settings"
  echo ""
fi

# Check if CMake is available
if ! command -v cmake &> /dev/null; then
  echo -e "${RED}Error: cmake not found in PATH${NC}"
  exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1)
echo -e "${GREEN}Found: $CMAKE_VERSION${NC}"
echo ""

# Check if submodules are initialized (needed for benchmarks)
if [ ! -d "third_party/benchmark/CMakeLists.txt" ]; then
  echo -e "${YELLOW}Warning: Google Benchmark submodule not found${NC}"
  echo "Initializing submodules..."
  git submodule update --init --recursive || {
    echo -e "${RED}Error: Failed to initialize submodules${NC}"
    exit 1
  }
  echo ""
fi

# Clean build directory to remove old CMake cache (especially important on Windows)
if [ -d "$BUILD_DIR" ]; then
  if [ -z "$QUIET" ]; then
    echo -e "${YELLOW}Cleaning build directory to remove old CMake cache...${NC}"
  fi
  rm -rf "$BUILD_DIR"
fi

# Configure CMake
echo -e "${GREEN}=== Configuring CMake ===${NC}"
CMAKE_ARGS=(
  -B "$BUILD_DIR"
  -G Ninja
  -DCMAKE_CXX_STANDARD=26
  -DCMAKE_CXX_STANDARD_REQUIRED=ON
  -DCMAKE_CXX_EXTENSIONS=OFF
  -DBUILD_BENCHMARKS=ON
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)

# This script uses Clang for building (required for clang-tidy)
# - Linux: Clang uses libstdc++ (GCC's standard library) - matches CI configuration
# - Windows: Clang uses MSVC's standard library (no -stdlib flag needed)

# Check if Clang is available
if ! command -v clang++ &> /dev/null; then
  echo -e "${RED}Error: Clang is required for clang-tidy${NC}"
  echo "Please install LLVM/Clang and ensure clang++ is in PATH"
  exit 1
fi

# Detect Windows: Git Bash sets MSYSTEM to MINGW64/MINGW32, or OSTYPE to msys
IS_WINDOWS=false
case "$OSTYPE" in
  msys|win32|cygwin)
    IS_WINDOWS=true
    ;;
esac
# Also check MSYSTEM (Git Bash sets this to MINGW64 or MINGW32)
if [ "$IS_WINDOWS" = "false" ] && [ -n "$MSYSTEM" ]; then
  case "$MSYSTEM" in
    MINGW*)
      IS_WINDOWS=true
      ;;
  esac
fi

# Configure Clang compiler
CMAKE_ARGS+=(
  -DCMAKE_C_COMPILER=clang
  -DCMAKE_CXX_COMPILER=clang++
)

# Linux: Clang uses libstdc++ (GCC's standard library) - matches CI configuration
# Windows: Clang uses MSVC's standard library, so no -stdlib flag needed
if [ "$IS_WINDOWS" = "false" ]; then
  CMAKE_ARGS+=(
    -DCMAKE_CXX_FLAGS="-stdlib=libstdc++"
  )
  if [ -z "$QUIET" ]; then
    echo -e "${GREEN}Using Clang with libstdc++ (GCC's standard library)${NC}"
  fi
else
  if [ -z "$QUIET" ]; then
    echo -e "${GREEN}Using Clang (Windows: uses MSVC standard library)${NC}"
  fi
fi

cmake "${CMAKE_ARGS[@]}" || {
  echo -e "${RED}Error: CMake configuration failed${NC}"
  exit 1
}
echo ""

# Build to generate complete compile_commands.json
echo -e "${GREEN}=== Building (to generate compile_commands.json) ===${NC}"
if [ -z "$QUIET" ]; then
  cmake --build "$BUILD_DIR" --parallel "$(nproc 2>/dev/null || echo 2)" || {
    echo -e "${YELLOW}Warning: Build had some errors, but continuing...${NC}"
  }
else
  cmake --build "$BUILD_DIR" --parallel "$(nproc 2>/dev/null || echo 2)" > /dev/null 2>&1 || {
    echo -e "${YELLOW}Warning: Build had some errors, but continuing...${NC}"
  }
fi
echo ""

# Verify compile_commands.json exists
if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
  echo -e "${RED}Error: compile_commands.json not found${NC}"
  echo "CMake configuration may have failed"
  exit 1
fi

ENTRY_COUNT=$(jq length "$BUILD_DIR/compile_commands.json" 2>/dev/null || echo "unknown")
echo -e "${GREEN}✓ compile_commands.json found ($ENTRY_COUNT entries)${NC}"
echo ""

# Find files to check
if [ -n "$FILES_SPECIFIED" ]; then
  # Use specified files
  FILES="$FILES_SPECIFIED"
  echo -e "${GREEN}=== Checking specified files ===${NC}"
  echo "Files: $FILES"
else
  # Find only .cpp files (exclude third_party and reference)
  # Note: Header files (.h, .hpp) are checked when included by .cpp files.
  # Checking header files directly causes "expected exactly one compiler job"
  # errors because clang-tidy treats them as precompiled headers.
  echo -e "${GREEN}=== Finding source files ===${NC}"
  FILES=$(find tests examples benchmarks -name "*.cpp" 2>/dev/null | \
    grep -v third_party | \
    grep -v reference || true)
  
  if [ -z "$FILES" ]; then
    echo -e "${YELLOW}No files found to check${NC}"
    exit 0
  fi
  
  FILE_COUNT=$(echo "$FILES" | wc -l)
  echo "Found $FILE_COUNT files to check"
fi
echo ""

# Run clang-tidy
echo -e "${GREEN}=== Running clang-tidy ===${NC}"
echo ""

CLANG_TIDY_ARGS=(
  -p "$BUILD_DIR"
  --config-file=.clang-tidy
)

if [ -n "$FIX_MODE" ]; then
  CLANG_TIDY_ARGS+=("$FIX_MODE")
  echo -e "${YELLOW}Note: Automatic fixes will be applied${NC}"
  echo ""
fi

if [ -n "$QUIET" ]; then
  CLANG_TIDY_ARGS+=("$QUIET")
fi

if [ -n "$VERBOSE" ]; then
  CLANG_TIDY_ARGS+=("$VERBOSE")
fi

# Run clang-tidy
if [ -z "$QUIET" ]; then
  echo "Command: clang-tidy [files] ${CLANG_TIDY_ARGS[*]}"
  echo ""
fi

# Count errors and warnings
ERROR_COUNT=0
WARNING_COUNT=0
TIDY_EXIT_CODE=0

# Run clang-tidy on each file individually to get detailed warnings
# This is necessary because clang-tidy only shows "warnings generated" when processing multiple files
TIDY_OUTPUT=""
FILE_NUM=0

for file in $FILES; do
  if [ ! -f "$file" ]; then
    continue
  fi
  
  FILE_NUM=$((FILE_NUM + 1))
  if [ -z "$QUIET" ]; then
    echo "[$FILE_NUM/$FILE_COUNT] Processing file $file..."
  fi
  
  # Run clang-tidy on single file to get detailed output
  FILE_OUTPUT=$(clang-tidy "$file" "${CLANG_TIDY_ARGS[@]}" 2>&1 || true)
  FILE_EXIT=$?
  
  # Track exit code (fail if any file fails)
  if [ $FILE_EXIT -ne 0 ]; then
    TIDY_EXIT_CODE=$FILE_EXIT
  fi
  
  # Filter out statistical noise (warnings generated, suppressed warnings, etc.)
  # Only keep actual warnings and errors
  FILTERED_OUTPUT=$(printf '%s' "$FILE_OUTPUT" | \
    grep -v "warnings generated" | \
    grep -v "Suppressed.*warnings" | \
    grep -v "Use -header-filter" | \
    grep -v "Use -system-headers" || true)
  
  # Only append if there are actual warnings/errors (not just statistics)
  if [ -n "$FILTERED_OUTPUT" ]; then
    TIDY_OUTPUT="${TIDY_OUTPUT}${FILTERED_OUTPUT}"$'\n'
  fi
done

# Display output (limit to first 50 lines if too long, or show summary)
if [ -z "$QUIET" ]; then
  # Show first 50 lines of actual warnings/errors (skip "warnings generated" lines)
  WARNING_LINES=$(printf '%s' "$TIDY_OUTPUT" | grep -E "(error:|warning:)" | head -50)
  if [ -n "$WARNING_LINES" ]; then
    echo -e "${YELLOW}=== Sample warnings/errors (first 50) ===${NC}"
    echo "$WARNING_LINES"
    echo ""
    TOTAL_WARNINGS=$(printf '%s' "$TIDY_OUTPUT" | grep -c "warning:" 2>/dev/null || echo "0")
    TOTAL_ERRORS=$(printf '%s' "$TIDY_OUTPUT" | grep -c "error:" 2>/dev/null || echo "0")
    # Remove any whitespace/newlines
    TOTAL_WARNINGS=$((TOTAL_WARNINGS + 0))
    TOTAL_ERRORS=$((TOTAL_ERRORS + 0))
    if [ "$TOTAL_WARNINGS" -gt 50 ] || [ "$TOTAL_ERRORS" -gt 50 ]; then
      echo -e "${YELLOW}... (showing first 50, total: $TOTAL_WARNINGS warnings, $TOTAL_ERRORS errors)${NC}"
      echo -e "${YELLOW}Use shell redirection to save full output: ./scripts/run_clang_tidy.sh > output.txt${NC}"
    fi
    echo ""
  else
    # No warnings/errors found, show full output
    echo "$TIDY_OUTPUT"
  fi
else
  # Quiet mode: only show errors
  ERROR_LINES=$(printf '%s' "$TIDY_OUTPUT" | grep "error:" || true)
  if [ -n "$ERROR_LINES" ]; then
    echo "$ERROR_LINES"
  fi
fi

# Count issues (rough estimate)
# Use grep + wc -l for reliable counting, then clean the result
ERROR_COUNT=$(printf '%s' "$TIDY_OUTPUT" | grep "error:" 2>/dev/null | wc -l)
WARNING_COUNT=$(printf '%s' "$TIDY_OUTPUT" | grep "warning:" 2>/dev/null | wc -l)

# Remove whitespace and ensure numeric (handle Windows line endings)
ERROR_COUNT=$((ERROR_COUNT + 0))
WARNING_COUNT=$((WARNING_COUNT + 0))

echo ""
echo -e "${GREEN}=== Summary ===${NC}"
if [ "$ERROR_COUNT" -gt 0 ] || [ "$WARNING_COUNT" -gt 0 ]; then
  echo -e "Errors: ${RED}$ERROR_COUNT${NC}"
  echo -e "Warnings: ${YELLOW}$WARNING_COUNT${NC}"
else
  echo -e "${GREEN}No issues found!${NC}"
fi
# Exit with appropriate code
if [ "$TIDY_EXIT_CODE" -ne 0 ]; then
  exit "$TIDY_EXIT_CODE"
fi

exit 0

