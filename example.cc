#include "bitvector.h"
#include <iostream>
#include <iomanip>

int main()
{
	stdex::bitvector v;
	std::cout << std::boolalpha
		<< "sizeof:\t\t\t" << sizeof(v) << std::endl
		<< "default ctor nothrow:\t"
		<< std::is_nothrow_default_constructible<
		    stdex::bitvector>::value << std::endl
		<< "default ctor size:\t" << v.size() << std::endl
		;
}
