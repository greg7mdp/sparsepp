#include <unordered_map>
#include <string>
#include <iostream>
#include <sparsepp/spp.h>

#include <sstream>

int main()
{
    using StringPtr = std::unique_ptr<std::string>;

#if 1
    using StringPtrContainer = spp::sparse_hash_map<StringPtr, StringPtr>;
#else
    using StringPtrContainer = std::unordered_map<StringPtr, StringPtr>;
#endif

    StringPtrContainer c;

    c.emplace(std::piecewise_construct, 
              std::forward_as_tuple(new std::string{ "key" }),
              std::forward_as_tuple(new std::string{ "value" }));
}

