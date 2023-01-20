long long int charPtrToInt(const char* str)
{
    long long int a = (long long int)str;
    return a;
}

int main() {
    const char* str = "123";
    charPtrToInt(str);
}
