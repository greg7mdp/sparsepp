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
    char buf[10], buf2[10];

    for (int i=0; i<10; ++i)
    {
        sprintf(buf, "%d", i); 
        sprintf(buf2, "val%d", i);
        auto& value = c[make_unique<std::string>(buf)];
        value = make_unique<std::string>(buf2);
    }

    for (int i=10; i<20; ++i)
    {
        sprintf(buf, "%d", i); 
        sprintf(buf2, "val%d", i);
        auto&& p = std::make_pair(make_unique<std::string>(buf), make_unique<std::string>(buf2));
        c.insert(std::move(p));
    }

    for (int i=20; i<30; ++i)
    {
        sprintf(buf, "%d", i); 
        sprintf(buf2, "val%d", i);
        c.emplace(std::make_pair(make_unique<std::string>(buf), make_unique<std::string>(buf2)));
    }

    for (auto& p : c)
        std::cout << *p.first << " -> " << *p.second << '\n';
}
