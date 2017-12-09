# Set some environment variables
export CCC_CC=clang-4.0
export CCC_CXX=clang++-4.0
CC_ANALYZER=/usr/lib/llvm-4.0/libexec/ccc-analyzer
CXX_ANALYZER=/usr/lib/llvm-4.0/libexec/c++-analyzer
# Create a separate build directory
rm -rf clang_sa_build
mkdir -p clang_sa_build
# Configure CMake
scan-build-4.0 --use-analyzer=$CXX_ANALYZER -disable-checker deadcode.DeadStores cmake -Bclang_sa_build -H. -DCMAKE_C_COMPILER=$CC_ANALYZER -DCCACHE_SUPPORT=OFF -DCMAKE_BUILD_TYPE=Debug
# Do a Clang build with static analyzer
make -C clang_sa_build clean
scan-build-4.0 --use-analyzer=$CXX_ANALYZER -o clang-code-analysis-report make -C clang_sa_build -j4
