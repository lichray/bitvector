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
#include <stdexcept>
#include <algorithm>
#include <numeric>

namespace stdex {

template <typename Allocator>
struct basic_bitvector
{
	typedef Allocator allocator_type;

private:
	typedef std::allocator_traits<allocator_type> _alloc_traits;
	typedef typename _alloc_traits::value_type _block_type;
	static_assert(std::is_unsigned<_block_type>(),
	    "underlying type must be unsigned");

	struct _blocks
	{
		_block_type* p;
		std::size_t cap;
	};

	static constexpr std::size_t bit_index(std::size_t n)
	{
		return n % _bits_per_block;
	}

	static constexpr _block_type bit_mask(std::size_t n)
	{
		return _block_type(1) << bit_index(n);
	}

	static constexpr auto _bits_internal = sizeof(_blocks) * CHAR_BIT;
	static constexpr auto _bits_per_block =
		std::numeric_limits<_block_type>::digits;
	static constexpr auto _bits_in_use = bit_mask(_bits_per_block - 1);

	using _bits = _block_type[_bits_internal / _bits_per_block];
	static_assert(sizeof(_bits) == sizeof(_blocks),
	    "unsupported representation");

public:

	struct reference
	{
	private:
		typedef typename basic_bitvector::_block_type& _bref_t;
		friend basic_bitvector;

		reference(_bref_t loc, std::size_t mask) :
			loc_(loc),
			mask_(mask)
		{}

	public:
		reference& operator=(bool value) noexcept
		{
			if (value)
				loc_ |= mask_;
			else
				loc_ &= ~mask_;

			return *this;
		}

		reference& operator=(reference other) noexcept
		{
			return (*this) = static_cast<bool>(other);
		}

		operator bool() const noexcept
		{
			return loc_ & mask_;
		}

		bool operator~() const noexcept
		{
			return !(*this);
		}

		reference& flip() noexcept
		{
			loc_ ^= mask_;
			return *this;
		}

	private:
		_bref_t loc_;
		std::size_t mask_;
	};

#define size_	sz_alloc_.first()
#define alloc_	sz_alloc_.second()
#define cap_	st_.blocks.cap
#define p_	st_.blocks.p
#define bits_	st_.bits
#define vec_	(using_bits() ? bits_ : p_)

	basic_bitvector() noexcept(
	    std::is_nothrow_default_constructible<allocator_type>()) :
		sz_alloc_(_bits_in_use)
	{}

	explicit basic_bitvector(allocator_type const& a) :
		sz_alloc_(_bits_in_use, a)
	{}

	basic_bitvector(basic_bitvector const& v) :
		sz_alloc_(v.size_, _alloc_traits::
		    select_on_container_copy_construction(v.alloc_))
	{
		// internal -> internal
		if (v.using_bits())
			st_ = v.st_;

		// heap -> internal
		else if (v.size() <= _bits_internal)
		{
			std::copy_n(v.p_, sizeof(bits_), bits_);
			size_ ^= _bits_in_use;
		}

		// heap -> shrunk heap
		else
		{
			allocate(aux::pow2_roundup(bits_to_count(v.size_)));
			copy_to_heap(v);
		}
	}

	// WIP: N2525
	basic_bitvector& operator=(basic_bitvector v)
	{
		swap(v);
		return *this;
	}

	~basic_bitvector() noexcept
	{
		if (not using_bits())
			deallocate();
	}

	reference operator[](std::size_t pos)
	{
		return { vec_[block_index(pos)], bit_mask(pos) };
	}

	bool operator[](std::size_t pos) const
	{
		return vec_[block_index(pos)] & bit_mask(pos);
	}

	bool test(std::size_t pos) const
	{
		if (pos >= size())
			throw std::out_of_range("basic_bitvector::test");

		return (*this)[pos];
	}

	std::size_t count() const noexcept
	{
		return std::accumulate(vec_, vec_ + block_index(size()),
		    std::size_t(0),
		    [](std::size_t n, _block_type v)
		    {
		    	return n + stdex::aux::popcount(v);
		    }
		    ) + stdex::aux::popcount(last_block());
	}

	bool empty() const noexcept
	{
		return size() == 0;
	}

	std::size_t size() const noexcept
	{
		return actual_size(size_);
	}

	std::size_t max_size() const noexcept
	{
		auto amax = _alloc_traits::max_size(alloc_);
		auto hmax = actual_size(std::numeric_limits<
		    std::size_t>::max());

		if (hmax / _bits_per_block <= amax)
			return hmax;
		else
			return count_to_bits(amax);
	}

	basic_bitvector& set(std::size_t pos, bool value = true)
	{
		if (pos >= size())
			throw std::out_of_range("basic_bitvector::set");

		set_bit_to(pos, value);
		return *this;
	}

	basic_bitvector& reset(std::size_t pos)
	{
		if (pos >= size())
			throw std::out_of_range("basic_bitvector::reset");

		unset_bit(pos);
		return *this;
	}

	basic_bitvector& flip(std::size_t pos)
	{
		if (pos >= size())
			throw std::out_of_range("basic_bitvector::flip");

		flip_bit(pos);
		return *this;
	}

	void push_back(bool value)
	{
		expand_to_hold(size() + 1);
		set_bit_to(size(), value);
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
	void set_bit_to(std::size_t pos, bool value)
	{
		if (value)
			set_bit(pos);
		else
			unset_bit(pos);
	}

	void set_bit(std::size_t pos)
	{
		vec_[block_index(pos)] |= bit_mask(pos);
	}

	void unset_bit(std::size_t pos)
	{
		vec_[block_index(pos)] &= ~bit_mask(pos);
	}

	void flip_bit(std::size_t pos)
	{
		vec_[block_index(pos)] ^= bit_mask(pos);
	}

	_block_type last_block() const
	{
		return vec_[block_index(size())] &
			(~_block_type(0) >>
			 (_bits_per_block - bit_index(size())));
	}

	bool using_bits() const
	{
		return size_ & _bits_in_use;
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

			basic_bitvector v(alloc_);
			v.allocate(aux::pow2_roundup(bits_to_count(sz)));
			v.size_ = size();
			v.copy_to_heap(*this);
			swap(v);
		}
	}

	void copy_to_heap(basic_bitvector const& v)
	{
		auto n = bits_to_count(size_);

		std::copy_n(v.using_bits() ? v.bits_ : v.p_, n, p_);
		std::fill_n(p_ + n, cap_ - n, _block_type(0));
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
#undef vec_

	static std::size_t count_to_bits(std::size_t n)
	{
		return n * _bits_per_block;
	}

	static std::size_t bits_to_count(std::size_t n)
	{
		return (n + (_bits_per_block - 1)) / _bits_per_block;
	}

	static std::size_t block_index(std::size_t n)
	{
		return n / _bits_per_block;
	}

	static std::size_t actual_size(std::size_t n)
	{
		return n & ~_bits_in_use;
	}

	union _ut
	{
		_blocks blocks;
		_bits bits;

		_ut() : bits() {}
	} st_;
	compressed_pair<std::size_t, allocator_type> sz_alloc_;
};

template <typename Allocator>
inline void swap(basic_bitvector<Allocator>& a, basic_bitvector<Allocator>& b)
	noexcept(noexcept(a.swap(b)))
{
	a.swap(b);
}

typedef basic_bitvector<std::allocator<unsigned long>> bitvector;

}

#endif
