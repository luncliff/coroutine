/**
 * @brief a source file to generate compiler macro
 * @note clang -dM test/a.cpp -E -std=c++20 1> clang-macro.txt
 * @note clang -dM test/a.cpp -E -std=c++17 -fcoroutines-ts 1> appleclang-cpp17-macro.txt
 * @note clang-cl /clang:-dM /clang:-std=c++20 test/a.cpp -E > docs/clang-cl-14-macro.txt
 */
int main(void) {
    return 0;
}
