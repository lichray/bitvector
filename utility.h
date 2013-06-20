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

#ifndef _UTILITY_H
#define _UTILITY_H 1

#include <utility>
#include <tuple>

namespace stdex {

using std::swap;

// assume T is swappable
template <typename T>
struct is_nothrow_swappable :
	std::integral_constant<bool,
	noexcept(swap(std::declval<T&>(), std::declval<T&>()))>
{};

template <typename T1, typename T2>
struct compressed_pair : std::tuple<T1, T2>
{
	typedef T1 first_type;
	typedef T2 second_type;

private:
	typedef std::tuple<T1, T2> _base;

public:
	constexpr compressed_pair() noexcept(
	    std::is_nothrow_default_constructible<_base>())
	{}

	constexpr explicit compressed_pair(first_type x) :
		_base(std::move(x), {})
	{}

	constexpr explicit compressed_pair(second_type y) :
		_base({}, std::move(y))
	{}

	constexpr compressed_pair(first_type x, second_type y) :
		_base(std::move(x), std::move(y))
	{}

	/* c++14 */ auto first() noexcept
		-> first_type&
	{
		return std::get<0>(*this);
	}

	constexpr auto first() const noexcept
		-> first_type const&
	{
		return std::get<0>(*this);
	}

	/* c++14 */ auto second() noexcept
		-> second_type&
	{
		return std::get<1>(*this);
	}

	constexpr auto second() const noexcept
		-> second_type const&
	{
		return std::get<1>(*this);
	}
};

}

#endif
