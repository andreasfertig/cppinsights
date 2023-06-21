namespace test {
typedef int jmp_buf[30];

jmp_buf a;
}


test::jmp_buf x;


using test::jmp_buf;

jmp_buf t;
