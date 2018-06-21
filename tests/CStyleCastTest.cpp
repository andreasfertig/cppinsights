#include <cassert>

int main() 
{
// XXX: Temp. disable as assert inserts the full path which is a problem on travis.
#if 0
    assert( 0 == 1);
#endif
}
