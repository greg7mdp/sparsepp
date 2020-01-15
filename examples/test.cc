#include <sparsepp/spp.h>
#include <utility>
#include <iostream>
#include <string>

auto bar() {
	return std::make_tuple(3, 4.5, "Some string");
}

int main()
{
	auto[c, d, e] = bar();
	std::cout << "c: " << c << ", d: " << d << ", e: " << e << "\n";

	spp::sparse_hash_set<int> test1;
	spp::sparse_hash_set<int> test2 = std::move(test1);
	test1.clear();
	test1.erase(0);
}
