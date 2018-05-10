int main() {
    int l1;
    int l2;

    [&]() {
        l1 = 2 * l2;
    }();
}
