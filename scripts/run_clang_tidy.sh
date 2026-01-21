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

# Configure CMake
echo -e "${GREEN}=== Configuring CMake ===${NC}"
CMAKE_ARGS=(
  -B "$BUILD_DIR"
  -DBUILD_BENCHMARKS=ON
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)

# Use clang if available (CI sets up clang/clang++ via update-alternatives)
if command -v clang++ &> /dev/null; then
  CMAKE_ARGS+=(
    -DCMAKE_C_COMPILER="$(which clang)"
    -DCMAKE_CXX_COMPILER="$(which clang++)"
  )
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
  # Find all .cpp, .hpp, .h files (exclude third_party and reference)
  echo -e "${GREEN}=== Finding source files ===${NC}"
  FILES=$(find include tests examples benchmarks -name "*.cpp" -o -name "*.hpp" -o -name "*.h" 2>/dev/null | \
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

# Run clang-tidy and capture output
TIDY_OUTPUT=$(clang-tidy $FILES "${CLANG_TIDY_ARGS[@]}" 2>&1 || true)
TIDY_EXIT_CODE=$?

# Display output
echo "$TIDY_OUTPUT"

# Count issues (rough estimate)
if echo "$TIDY_OUTPUT" | grep -q "error:"; then
  ERROR_COUNT=$(echo "$TIDY_OUTPUT" | grep -c "error:" || echo "0")
fi
if echo "$TIDY_OUTPUT" | grep -q "warning:"; then
  WARNING_COUNT=$(echo "$TIDY_OUTPUT" | grep -c "warning:" || echo "0")
fi

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
