#include <cstdio>
#include <csetjmp>
std::jmp_buf jump_buffer;

void foo() {
    printf("foo: Entered\n");
    longjmp(jump_buffer, 1);  // Jump back to where setjmp was called
    printf("foo: This won't be executed\n");
}

int main() {
    if (setjmp(jump_buffer) == 0) {
        printf("main: Calling foo\n");
        foo();
    } else {
        printf("main: Jumped back from foo\n");
    }

    printf("main: Exiting\n");
    return 0;
}
