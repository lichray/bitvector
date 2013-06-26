/*-
 * Copyright (c) 2013 Zhihao Yuan.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef ___AUX_H
#define ___AUX_H 1

#include <climits>
#include <limits>
#include <type_traits>

namespace stdex {
namespace aux {

template <int bit>
struct or_shift
{
	template <typename Int>
	static constexpr auto apply(Int n) -> Int
	{
		return or_shift<bit / 2>::apply(n | (n >> bit));
	}
};

template <>
struct or_shift<1>
{
	template <typename Int>
	static constexpr auto apply(Int n) -> Int
	{
		return n | (n >> 1);
	}
};

template <typename Int, typename R = typename std::make_unsigned<Int>::type>
constexpr auto pow2_roundup(Int n) -> R
{
	return or_shift<
	    std::numeric_limits<R>::digits / 2>::apply(R(n) - 1) + 1;
}

template <int bit>
struct fill_bits
{
	template <typename Int>
	static constexpr auto apply(Int n) -> Int
	{
		return _lambda(fill_bits<bit / 2>::apply(n));
	}

private:
	template <typename Int>
	static constexpr auto _lambda(Int x) -> Int
	{
		return (x << (bit / 2)) ^ x;
	}
};

template <>
struct fill_bits<CHAR_BIT>
{
	template <typename Int>
	static constexpr auto apply(Int n) -> Int
	{
		return n;
	}
};

template <typename Int>
constexpr auto magic(Int n)
	-> typename std::enable_if<std::is_unsigned<Int>::value, Int>::type
{
	return fill_bits<std::numeric_limits<Int>::digits>::apply(n);
}

template <typename Int>
struct m1 : std::integral_constant<Int, magic(Int(0x55))> {};

template <typename Int>
struct m2 : std::integral_constant<Int, magic(Int(0x33))> {};

template <typename Int>
struct m4 : std::integral_constant<Int, magic(Int(0x0f))> {};

template <typename Int>
struct h01 : std::integral_constant<Int, magic(Int(0x01))> {};

template <typename Int>
/* c++14 */ inline auto popcount(Int x) -> std::size_t
{
	x -= (x >> 1) & m1<Int>();
	x = (x & m2<Int>()) + ((x >> 2) & m2<Int>());
	x = (x + (x >> 4)) & m4<Int>();
	return Int(x * h01<Int>()) >>
	    (std::numeric_limits<Int>::digits - CHAR_BIT);
}

}
}

#endif
