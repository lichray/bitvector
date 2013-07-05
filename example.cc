#include "bitvector.h"
#include <iostream>
#include <iomanip>

int main()
{
	stdex::bitvector v;
	std::cout << std::boolalpha;

	std::cout
		<< "sizeof:\t\t\t" << sizeof(v) << std::endl
		<< "max size:\t\t" << v.max_size() << std::endl
		<< "nothrow default ctor:\t"
		<< std::is_nothrow_default_constructible<
		    stdex::bitvector>() << std::endl
		<< "nothrow swap:\t\t" << noexcept(swap(v, v)) << std::endl
		<< "default init size:\t" << v.size() << std::endl
		;

	std::cout << std::noboolalpha;

	for (int i = 0; i < 129; ++i)
		v.push_back(true);

	std::cout
		<< "size after insertion:\t" << v.size() << std::endl
		<< "last bit:\t\t" << v.test(128) << std::endl
		;

	v.set(128, false);
	std::cout << "after set to false:\t" << v[128] << std::endl;

	v.flip(128);
	std::cout << "after flipped:\t\t" << v[128] << std::endl;

	v.reset(128);
	std::cout << "after reset:\t\t" << v[128] << std::endl;

	auto r = v[128];
	r.flip();

	std::cout
		<< "after proxy flipped:\t" << v[128] << std::endl
		<< "proxy:\t\t\t" << r << std::endl
		<< "proxy inversed:\t\t" << ~r << std::endl
		;

	v[127].flip();
	stdex::bitvector v2(v);

	std::cout
		<< "popcount after a flip:\t" << v.count() << std::endl
		<< "popcount of copied:\t" << v2.count() << std::endl
		;

	stdex::bitvector v3;
	v = v3;

	std::cout
		<< "popcount after assign:\t" << v.count() << std::endl
		;

	v = std::move(v2);

	std::cout
		<< "after moved back:\t" << v.count() << std::endl
		;

	std::cout << std::boolalpha;

	for (int i = 0; i < 128; ++i)
		v.reset(i);

	std::cout << "any with last bit set:\t" << v.any() << std::endl;

	v.reset(128);
	std::cout << "any with no bit set:\t" << v.any() << std::endl;
}
