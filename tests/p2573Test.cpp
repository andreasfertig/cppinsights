// cmdline:-std=c++26

void newapi();
void oldapi() = delete("This old API is outdated and already been removed. Please use newapi() instead.");
