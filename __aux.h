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
#include <iterator>
#include <memory>

namespace stdex {
namespace aux {

template <int N>
struct or_shift
{
	template <typename Int>
	static constexpr auto apply(Int n) -> Int
	{
		return or_shift<N / 2>::apply(n | (n >> N));
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

template <int N>
struct fill_bits
{
	template <typename Int>
	static constexpr auto apply(Int n) -> Int
	{
		return _lambda(fill_bits<N / 2>::apply(n));
	}

private:
	template <typename Int>
	static constexpr auto _lambda(Int x) -> Int
	{
		return (x << (N / 2)) ^ x;
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

template <std::size_t I, std::size_t N>
struct set_bit1_loop
{
	template <typename Int, typename RandomAccessIterator, typename T>
	static void apply(Int i, RandomAccessIterator it, T one,
	    std::random_access_iterator_tag tag)
	{
		if (i & (Int(1) << (N - I)))
			it[I] = one;
		set_bit1_loop<I + 1, N>::apply(i, it, one, tag);
	}
};

template <std::size_t N>
struct set_bit1_loop<N, N>
{
	template <typename Int, typename RandomAccessIterator, typename T>
	static void apply(Int i, RandomAccessIterator it, T one,
	    std::random_access_iterator_tag)
	{
		if (i & Int(1))
			it[N] = one;
	}
};

template <int Width, typename Int, typename Iter, typename T>
inline void fill_bit1_impl(Int i, Iter it, T one)
{
	set_bit1_loop<0, Width - 1>::apply(i, it, one,
	    typename std::iterator_traits<Iter>::iterator_category());
}

template <typename Int, typename Iter, typename T>
inline void fill_bit1(Int i, Iter it, T one)
{
	constexpr auto digits = std::numeric_limits<Int>::digits;
	fill_bit1_impl<digits>(i, it, one);
}

template <int Low, int High, int Mid = (Low + High) / 2,
	  typename = void>
struct fill_bit1_upto_impl;

template <int Low, int High, int Mid>
struct fill_bit1_upto_impl<Low, High, Mid,
	typename std::enable_if<(Low > High)>::type>
{
	template <typename Int, typename Iter, typename T>
	static void apply(size_t n, Int i, Iter it, T one)
	{
		throw 1;
	}
};

template <int Mid>
struct fill_bit1_upto_impl<Mid, Mid, Mid, void>
{
	template <typename Int, typename Iter, typename T>
	static void apply(size_t n, Int i, Iter it, T one)
	// precondition: n == Mid
	{
		fill_bit1_impl<Mid>(i, it, one);
	}
};

template <int Low, int High, int Mid>
struct fill_bit1_upto_impl<Low, High, Mid,
	typename std::enable_if<(Low < High)>::type>
{
	template <typename Int, typename Iter, typename T>
	static void apply(size_t n, Int i, Iter it, T one)
	// precondition: Low <= n or n <= High
	{
		if (n < Mid)
			fill_bit1_upto_impl<Low, Mid - 1>::apply(n, i, it, one);
		else if (n == Mid)
			fill_bit1_impl<Mid>(i, it, one);
		else
			fill_bit1_upto_impl<Mid + 1, High>::apply(n, i, it, one);
	}
};

template <typename Int, typename Iter, typename T>
inline void fill_bit1_upto(size_t n, Int i, Iter it, T one)
// precondition: 0 < n <= bitsof(Int)
{
	constexpr auto digits = std::numeric_limits<Int>::digits;
	fill_bit1_upto_impl<1, digits>::apply(n, i, it, one);
}

template <unsigned char Byte, int I>
struct parse_byte_impl
{
	template <typename Iter, typename UnaryPredicate>
	static auto apply(Iter it, UnaryPredicate f) -> unsigned char
	{
		if (f(*it))
			return parse_byte_impl<(Byte << 1) ^ 1, I + 1>::apply(
			    ++it, f);
		else
			return parse_byte_impl<(Byte << 1), I + 1>::apply(
			    ++it, f);
	}

	template <typename Iter, typename UnaryPredicate>
	static auto apply(Iter it, Iter ed, UnaryPredicate f) -> unsigned char
	// precondition: distance(it, ed) <= CHAR_BIT
	{
		if (it == ed)
			return Byte;
		if (f(*it))
			return parse_byte_impl<(Byte << 1) ^ 1, I + 1>::apply(
			    ++it, ed, f);
		else
			return parse_byte_impl<(Byte << 1), I + 1>::apply(
			    ++it, ed, f);
	}
};

template <unsigned char Byte>
struct parse_byte_impl<Byte, CHAR_BIT>
{
	template <typename Iter, typename UnaryPredicate>
	static auto apply(Iter it, UnaryPredicate f) -> unsigned char
	{
		return Byte;
	}

	template <typename Iter, typename UnaryPredicate>
	static auto apply(Iter it, Iter ed, UnaryPredicate f) -> unsigned char
	// precondition: it == ed
	{
		return Byte;
	}
};

template <typename Iter, typename UnaryPredicate>
inline auto parse_byte(Iter it, UnaryPredicate f) -> unsigned char
{
	return parse_byte_impl<0, 0>::apply(it, f);
}

template <typename Iter, typename UnaryPredicate>
inline auto parse_byte(Iter it, Iter ed, UnaryPredicate f) -> unsigned char
{
	return parse_byte_impl<0, 0>::apply(it, ed, f);
}

}

template <typename Alloc1, typename Alloc2>
struct same_allocator : std::integral_constant<bool,
	std::is_same<typename std::allocator_traits<Alloc2>::template
	    rebind_alloc<typename std::allocator_traits<Alloc1>::value_type>,
	    Alloc1>::value>
{};

template <typename Iter>
inline auto reverser(Iter it) -> std::reverse_iterator<Iter>
{
	return std::reverse_iterator<Iter>(it);
}

}

#endif
