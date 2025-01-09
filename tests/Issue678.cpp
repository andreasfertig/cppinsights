#include <cstdio>

int next() {
    static int i = 1;
    return i++;
}

int main() {
    while (int i = next()) {
        printf("i=%d\n", i);
        if (i >= 10) {
            break;
        }
    }
}
