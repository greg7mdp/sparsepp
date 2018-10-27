#include <iostream>
#include "sparsepp/spp.h"
#include <unordered_map>

using namespace std;

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

int main()
{
    using Key = std::unique_ptr<std::string>;
    using Value = std::unique_ptr<std::string>;

#if 1
    using AssociativeContainer = spp::sparse_hash_map<Key, Value>;
#else
    using AssociativeContainer = unordered_map<Key, Value>;
#endif

    AssociativeContainer c;
    char buf[10];

    for (int i=0; i<100; ++i)
    {
        sprintf(buf, "%d", i);
        auto& value = c[make_unique<std::string>(buf)];
        sprintf(buf, "val%d", i);
        value = make_unique<std::string>(buf);
    }

    for (auto& p : c)
        std::cout << *p.first << " -> " << *p.second << '\n';
}
