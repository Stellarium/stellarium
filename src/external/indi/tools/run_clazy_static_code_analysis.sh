# Create a separate build directory
rm -rf clazy_build
mkdir -p clazy_build
# Configure CMake
export CLANGXX=clang++-3.9
export CLAZY_CHECKS="level0,level1,no-non-pod-global-static,no-container-anti-pattern"
cmake -Bclazy_build -H. -DCMAKE_C_COMPILER=clang-3.9 -DCMAKE_CXX_COMPILER=clazy -DCCACHE_SUPPORT=OFF -DCMAKE_BUILD_TYPE=Debug
# Do a Clang build with Clazy static analyzer
make -C clazy_build -j4
