// Test that it's possible to use spp with relative includes only - without adding -I to the compiler
#include "../sparsepp/spp.h"

int main()
{
    spp::sparse_hash_map<unsigned, unsigned> dummy;
}
