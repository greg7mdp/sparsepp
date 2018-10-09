// ---------------------------------------------------------------------------
// values inserted into sparsepp have to be either copyable or movable.
// this example demonstrates inserting a movable only object.
// ---------------------------------------------------------------------------

#include <sparsepp/spp.h>
#include <string>
#include <memory>

class A 
{
public:
    A(const std::pair<std::string, std::string>& b) 
    {
        key   = b.first;
        value = b.second;
    }

    // no copy allowed
    A(const A&) = delete;
    A& operator=(const A&) = delete;

    // but moving is OK
    A(A&&) = default;
    A& operator=(A&&) = default;

private:
    std::string key;
    std::string value;
};

int main() 
{
    spp::sparse_hash_map<std::string, A> m;

    m.emplace(std::piecewise_construct,
              std::forward_as_tuple("c"),
              std::forward_as_tuple(std::make_pair("a", "bcd")));

    return 0;
}
