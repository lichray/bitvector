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

#ifndef _BITVECTOR_H
#define _BITVECTOR_H 1

#include <memory>
#include <limits>
#include <climits>
#include <bitset>
#include <boost/compressed_pair.hpp>

namespace stdex {

template <typename Allocator>
struct basic_bitvector
{
	typedef Allocator allocator_type;

private:
	typedef std::allocator_traits<allocator_type> _alloc_traits;
	typedef typename _alloc_traits::value_type _block_type;
	static_assert(std::is_unsigned<_block_type>::value,
	    "underlying type must be unsigned");

	struct _blocks {
		_block_type* p;
		std::size_t cap;
	};

	constexpr static auto _bits_internal = sizeof(_blocks) * CHAR_BIT;
	constexpr static auto _bits_per_block =
		std::numeric_limits<_block_type>::digits;

	typedef std::bitset<_bits_internal> _bits;
	static_assert(sizeof(_bits) == sizeof(_blocks),
	    "bitset is larger than expected");

public:

#define _size	sz_alloc_.first()
#define _alloc	sz_alloc_.second()

	basic_bitvector() noexcept(
	    std::is_nothrow_default_constructible<allocator_type>::value) :
		sz_alloc_(0)
	{}

	bool empty() const noexcept
	{
		return _size == 0;
	}

	std::size_t size() const noexcept
	{
		return _size;
	}

	std::size_t max_size() const noexcept
	{
		auto amax = _alloc_traits::max_size(_alloc);
		auto hmax = std::numeric_limits<
		    typename _alloc_traits::difference_type>::max();

		if (hmax / _bits_per_block <= amax)
			return hmax;
		else
			return amax * CHAR_BIT;
	}

	std::size_t capacity() const noexcept
	{
		if (_still_short())
			return _bits_internal;
		else
			return st_.blocks.cap * CHAR_BIT;
	}


private:
	bool _still_short() const
	{
		return _size <= _bits_internal;
	}

#undef _size
#undef _alloc

	union _ut {
		_blocks blocks;
		_bits bits;

		_ut() {}
	} st_;
	boost::compressed_pair<std::size_t, allocator_type> sz_alloc_;
};

typedef basic_bitvector<std::allocator<unsigned long>> bitvector;

}

#endif
