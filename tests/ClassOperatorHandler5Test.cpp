#include <string>

std::string trim(const std::string& input)
{
    if(input.length() == 0) {
        return "";
    }

    std::string final = input;

    int i = 0;
    while(i < (int)input.length() && input[i] <= ' ') {
        i++;
    }

    if(i >= (int)input.length()) {
        return "";
    }

    if(i > 0) {
        final = input.substr(i, input.length() - i);
    }

    i = (int)final.length() - 1;
    while(i >= 0 && final[i] <= ' ') {
        i--;
    }

    return final.substr(0, i + 1);
}

int main()
{
    std::string x{" ashj a a a aaa"};

    std::string b = trim(x);
}
