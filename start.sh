#!/bin/bash

# Initialize flag variables
REBUILD=false
RECOMPILE_SHADERS=false

# Function to display help information
show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -b    Rebuild the project"
    echo "  -s    Recompile all shader files"
    echo "  -h    Show this help message"
}

# Parse command-line options
while getopts "bsh" opt; do
  case $opt in
    b)
      REBUILD=true
      ;;
    s)
      RECOMPILE_SHADERS=true
      ;;
    h)
      show_help
      exit 0
      ;;
    \?)
      show_help
      exit 1
      ;;
  esac
done

cd "$(dirname "$0")"

# Check if the build directory exists
if [ ! -d "build" ]; then
    REBUILD=true
fi

# Decide whether to rebuild based on the REBUILD flag
if [ "$REBUILD" = true ]; then
  rm -rf build
  mkdir build
  cd build
  cmake ..
  cmake --build .

  if [ $? -ne 0 ]; then
    exit 1
  fi
  cd ..
fi

# Decide whether to recompile shaders based on the -s option
if [ "$RECOMPILE_SHADERS" = true ]; then
  # Delete all .spv files in assets/shaders to tell shader2spirv recompile
  find assets/shaders -type f -name "*.spv" -delete
fi

sh assets/shaders/shader2spirv.sh
if [ $? -ne 0 ]; then
    exit 1
fi

build/Venom