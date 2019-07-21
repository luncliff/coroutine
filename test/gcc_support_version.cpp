
#include <iostream>

using namespace std;

int main(int, char* []) {
    cout << "c++        : " << __cplusplus << endl;      // --std=c++1z
    cout << "coroutines : " << __cpp_coroutines << endl; // -fcoroutines
    cout << "concepts   : " << __cpp_concepts << endl;   // -fconcepts
    return 0;
}
