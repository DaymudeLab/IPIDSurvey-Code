set -e

# Get benchmark root directory.
DIR=$(cd "$(dirname ${BASH_SOURCE[0]})" && pwd)

# Create build dir.
echo "Creating build directory..."
mkdir -p ${DIR}/build
cd ${DIR}/build

# Configure and run the build process using CMake.
echo "Building ipid_bench..."
cmake ..
cmake --build .
cd ..
