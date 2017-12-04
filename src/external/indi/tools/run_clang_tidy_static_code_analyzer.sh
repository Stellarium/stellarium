
ARGS1="-checks=*,-google-readability-casting,-google-readability-braces-around-statements,-readability-braces-around-statements"
ARGS2="-cppcoreguidelines-pro-type-vararg,-clang-diagnostic-unused-command-line-argument,-cppcoreguidelines-no-malloc"
ARGS3="-cert-msc30-c,-google-readability-todo,-cppcoreguidelines-pro-bounds-array-to-pointer-decay,-google-build-using-namespace"
ARGS4="-misc-macro-parentheses,-modernize-use-using,-llvm-include-order,-readability-static-definition-in-anonymous-namespace"
ARGS5="-cert-err58-cpp,-readability-else-after-return,-cppcoreguidelines-pro-bounds-pointer-arithmetic"
ARGS6="-cppcoreguidelines-pro-type-reinterpret-cast,-readability-redundant-member-init,-cppcoreguidelines-pro-bounds-constant-array-index"
ARGS7="-clang-analyzer-deadcode.DeadStores,-google-runtime-int,-cert-err34-c"

# Configure CMake
cmake -Bclang_tidy_build -H. -DCMAKE_CXX_CLANG_TIDY:STRING="clang-tidy-4.0;$ARGS1,$ARGS2,$ARGS3,$ARGS4,$ARGS5,$ARGS6,$ARGS7" \
      -DUNITY_BUILD=ON -DCMAKE_C_COMPILER=clang-4.0 -DCMAKE_CXX_COMPILER=clang++-4.0 $@

make -C clang_tidy_build -j4
