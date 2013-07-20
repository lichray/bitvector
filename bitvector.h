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
#include <string>

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

	typedef _block_type* _block_iterator;
	typedef _block_type const* _block_const_iterator;

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

	using _zeros = std::integral_constant<_block_type, 0>;
	using _ones = std::integral_constant<_block_type, ~_zeros()>;

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

	explicit basic_bitvector(std::size_t n,
	    allocator_type const& a = allocator_type()) :
		sz_alloc_(_bits_in_use, a)
	{
		expand_to_hold(n);
		size_ ^= n;

		reset();
	}
	
	basic_bitvector(std::size_t n, bool const& value,
	    allocator_type const& a = allocator_type()) :
		sz_alloc_(_bits_in_use, a)
	{
		expand_to_hold(n);
		size_ ^= n;

		if (value)
			set();
		else
			reset();
	}

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

	basic_bitvector(basic_bitvector&& v) noexcept(
	    std::is_nothrow_move_constructible<allocator_type>()) :
		st_(v.st_),
		sz_alloc_(v.sz_alloc_)
	{
		// minimal change to prevent deallocation
		v.size_ = _bits_in_use;
	}

	~basic_bitvector() noexcept
	{
		if (not using_bits())
			deallocate();
	}

	// WIP: N2525
	basic_bitvector& operator=(basic_bitvector v)
	{
		swap(v);
		return *this;
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

	bool all() const noexcept
	{
		bool r = std::none_of(begin(), filled_end(),
		    [](_block_type v) -> bool
		    {
			return ~v;
		    });

		if (!r)
			return false;
		else
			return not has_incomplete_block()
				or !dezeroed_last_block();
	}

	bool any() const noexcept
	{
		bool r = std::any_of(begin(), filled_end(),
		    [](_block_type v) -> bool
		    {
			return v;
		    });

		if (r)
			return true;
		else
			return has_incomplete_block() and zeroed_last_block();
	}

	bool none() const noexcept
	{
		return not any();
	}

	std::size_t count() const noexcept
	{
		auto n = std::accumulate(begin(), filled_end(),
		    std::size_t(0),
		    [](std::size_t n, _block_type v)
		    {
			return n + stdex::aux::popcount(v);
		    });

		if (has_incomplete_block())
			return n + stdex::aux::popcount(zeroed_last_block());
		else
			return n;
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

	basic_bitvector& set() noexcept
	{
		std::fill(begin(), end(), _ones());
		return *this;
	}

	basic_bitvector& set(std::size_t pos, bool value = true)
	{
		if (pos >= size())
			throw std::out_of_range("basic_bitvector::set");

		set_bit_to(pos, value);
		return *this;
	}

	basic_bitvector& reset() noexcept
	{
		std::fill(begin(), end(), _zeros());
		return *this;
	}

	basic_bitvector& reset(std::size_t pos)
	{
		if (pos >= size())
			throw std::out_of_range("basic_bitvector::reset");

		unset_bit(pos);
		return *this;
	}

	basic_bitvector& flip() noexcept
	{
		std::transform(begin(), end(),
		    begin(),
		    [](_block_type v)
		    {
			return ~v;
		    });

		return *this;
	}

	basic_bitvector& flip(std::size_t pos)
	{
		if (pos >= size())
			throw std::out_of_range("basic_bitvector::flip");

		flip_bit(pos);
		return *this;
	}

	void clear() noexcept
	{
		size_ &= _bits_in_use;
	}

	void push_back(bool value)
	{
		expand_to_hold(size() + 1);
		set_bit_to(size(), value);
		++size_;
	}

	void pop_back()
	{
		--size_;
	}

	void resize(std::size_t n, bool value = false)
	{
		auto oldn = bits_to_count(size());
		auto newn = bits_to_count(n);

		expand_to_hold(n);
		if (has_incomplete_block() and size() < n)
			last_block() = value ?
				oned_last_block() :
				zeroed_last_block();

		size_ = (size_ & _bits_in_use) ^ n;
		if (oldn < newn)
			std::fill(begin() + oldn, end(),
			    value ? _ones() : _zeros());
	}

	void swap(basic_bitvector& v) noexcept(
	    is_nothrow_swappable<allocator_type>())
	{
		using std::swap;

		swap(alloc_, v.alloc_);
		swap(size_, v.size_);
		swap(st_, v.st_);
	}

	template <typename charT = char,
		  typename traits = std::char_traits<charT>,
		  typename _Allocator = std::allocator<charT>>
	std::basic_string<charT, traits, _Allocator>
	to_string(charT zero = charT('0'), charT one = charT('1')) const
	{
		std::basic_string<charT, traits, _Allocator> s(size(), zero);
		auto it = s.begin();

		if (has_incomplete_block())
		{
			aux::fill_bit1_upto(extra_size(),
			    last_block(), it, one);
			it += extra_size();
		}

		std::for_each(reverser(filled_end()), reverser(begin()),
		    [&](_block_type v)
		    {
			aux::fill_bit1(v, it, one);
			it += _bits_per_block;
		    });

		return s;
	}

	unsigned long to_ulong() const
	{
		if (size() > std::numeric_limits<unsigned long>::digits)
			throw std::overflow_error("basic_bitvector::to_ulong");

		return as_integral<unsigned long>();
	}

	unsigned long long to_ullong() const
	{
		if (size() > std::numeric_limits<unsigned long long>::digits)
			throw std::overflow_error("basic_bitvector::to_ullong");

		return as_integral<unsigned long long>();
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

	bool has_incomplete_block() const
	{
		return extra_size() != 0;
	}

	std::size_t extra_size() const
	{
		return bit_index(size());
	}

	_block_type& last_block()
	{
		return *filled_end();
	}

	_block_type last_block() const
	{
		return *filled_end();
	}

	_block_type zeroed_last_block() const
	{
		return last_block() & extra_mask();
	}

	_block_type dezeroed_last_block() const
	{
		return ~last_block() & extra_mask();
	}

	_block_type oned_last_block() const
	{
		return last_block() | ~extra_mask();
	}

	_block_type extra_mask() const
	{
		return _ones() >> (_bits_per_block - extra_size());
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

		std::copy_n(v.begin(), n, p_);
		std::fill_n(p_ + n, cap_ - n, _zeros());
	}

	_block_const_iterator begin() const
	{
		return vec_;
	}

	_block_const_iterator filled_end() const
	{
		return vec_ + block_index(size());
	}

	_block_iterator begin()
	{
		return vec_;
	}

	_block_iterator filled_end()
	{
		return vec_ + block_index(size());
	}

	_block_iterator end()
	{
		return vec_ + bits_to_count(size());
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

	template <typename R>
	R as_integral() const
	{
		return as_integral<R>(std::integral_constant<bool,
		    std::is_convertible<_block_type, R>()
		    and sizeof(_block_type) >= sizeof(R)>());
	}

	template <typename R>
	R as_integral(std::true_type) const
	{
		return zeroed_last_block();
	}

	template <typename R>
	R as_integral(std::false_type) const
	{
		if (begin() == filled_end())
			return zeroed_last_block();

		auto r =
		    std::accumulate(begin() + 1, filled_end(),
		    R(vec_[0]),
		    [](R r, _block_type v)
		    {
			return r ^ (R(v) << _bits_per_block);
		    });

		if (has_incomplete_block())
			return r ^ (R(zeroed_last_block()) << _bits_per_block);
		else
			return r;
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
