#include <iostream>
#include <cstdint>

int main() {
    int16_t a = (int16_t)0x8000;
    std::cout << a << std::endl;
    std::cout << (uint32_t)a << std::endl;
    std::cout << (uint32_t)(uint16_t)a << std::endl;
}
