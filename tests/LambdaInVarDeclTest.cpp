int main()
{
    char buffer[4];

    for(auto& c: buffer) {
        int x = [&]() {
            return c;
        }();
    }
}

