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

#include "utility.h"
#include "__aux.h"
#include <climits>
#include <boost/compressed_pair.hpp>

namespace stdex {

template <typename Allocator>
struct basic_bitvector
{
	typedef Allocator allocator_type;

private:
	typedef basic_bitvector<allocator_type> _self;
	typedef std::allocator_traits<allocator_type> _alloc_traits;
	typedef typename _alloc_traits::value_type _block_type;
	static_assert(std::is_unsigned<_block_type>(),
	    "underlying type must be unsigned");

	struct _blocks {
		_block_type* p;
		std::size_t cap;
	};

	constexpr static auto _bits_internal = sizeof(_blocks) * CHAR_BIT;
	constexpr static auto _bits_per_block =
		std::numeric_limits<_block_type>::digits;

	using _bits = _block_type[_bits_internal / _bits_per_block];
	static_assert(sizeof(_bits) == sizeof(_blocks),
	    "unsupported pointer size");

public:

#define size_	sz_alloc_.first()
#define alloc_	sz_alloc_.second()
#define cap_	st_.blocks.cap
#define p_	st_.blocks.p
#define bits_	st_.bits

	basic_bitvector() noexcept(
	    std::is_nothrow_default_constructible<allocator_type>()) :
		sz_alloc_(0)
	{}

	explicit basic_bitvector(allocator_type const& a) :
		sz_alloc_(0, a)
	{}

	~basic_bitvector() noexcept
	{
		if (not using_bits() and p_ != nullptr)
			deallocate();
	}

	bool empty() const noexcept
	{
		return size_ == 0;
	}

	std::size_t size() const noexcept
	{
		return size_;
	}

	std::size_t max_size() const noexcept
	{
		auto amax = _alloc_traits::max_size(alloc_);
		auto hmax = std::numeric_limits<std::size_t>::max();

		if (hmax / _bits_per_block <= amax)
			return hmax;
		else
			return count_to_bits(amax);
	}

	void push_back(bool b)
	{
		expand_to_hold(size_ + 1);
		++size_;
	}

	void swap(basic_bitvector& v) noexcept(
	    is_nothrow_swappable<allocator_type>())
	{
		using std::swap;

		swap(alloc_, v.alloc_);
		swap(size_, v.size_);
		swap(st_, v.st_);
	}

private:
	bool using_bits() const
	{
		return size_ <= _bits_internal;
	}

	std::size_t capacity() const
	{
		if (using_bits())
			return _bits_internal;
		else
			return count_to_bits(cap_);
	}

	void expand_to_hold(std::size_t sz)
	{
		if (sz > capacity()) {
			if (sz > max_size())
				throw std::length_error("bitvector");

			_self v(alloc_);
			v.allocate(aux::pow2_roundup(bits_to_count(sz)));
			// fake copying
			v.size_ = size_;
			swap(v);
		}
	}

	void allocate(std::size_t n)
	{
		p_ = _alloc_traits::allocate(alloc_, n);
		cap_ = n;
	}

	void deallocate()
	{
		_alloc_traits::deallocate(alloc_, p_, cap_);
	}

#undef size_
#undef alloc_
#undef cap_
#undef p_
#undef bits_

	static std::size_t count_to_bits(std::size_t n)
	{
		return n * _bits_per_block;
	}

	static std::size_t bits_to_count(std::size_t n)
	{
		return (n + (_bits_per_block - 1)) / _bits_per_block;
	}

	union _ut {
		_blocks blocks;
		_bits bits;

		_ut() : bits() {}
	} st_;
	boost::compressed_pair<std::size_t, allocator_type> sz_alloc_;
};

template <typename Allocator>
void swap(basic_bitvector<Allocator>& a, basic_bitvector<Allocator>& b)
	noexcept(noexcept(a.swap(b)))
{
	a.swap(b);
}

typedef basic_bitvector<std::allocator<unsigned long>> bitvector;

}

#endif
