#include "bitvector.h"
#include <iostream>
#include <iomanip>

int main()
{
	stdex::bitvector v;
	std::cout << std::boolalpha;

	std::cout
		<< "sizeof:\t\t\t" << sizeof(v) << std::endl
		<< "max_size:\t\t" << v.max_size() << std::endl
		<< "nothrow default ctor:\t"
		<< std::is_nothrow_default_constructible<
		    stdex::bitvector>::value << std::endl
		<< "nothrow swap:\t\t" << noexcept(swap(v, v)) << std::endl
		<< "default init size:\t" << v.size() << std::endl

		;
	for (int i = 0; i < 129; ++i)
		v.push_back(true);

	std::cout
		<< "size after insertion:\t" << v.size() << std::endl
		;
}
