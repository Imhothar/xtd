/*
 Copyright 2013 Miro Knejp
 
 See the accompanied LICENSE file for licensing details.
 */

/**
 \file
 Partial implementation of the string_view proposal N3609 with some discrepancies.
 
 Reference: http://htmlpreview.github.io/?https://github.com/google/cxx-std-draft/blob/string-ref-paper/string_view.html
 This implementation uses xtd::optional to avoid the "npos" hack.
 
 \author Miro Knejp
 */

#ifndef XTD_xtd_string_44929f79_d6b5_452e_9c17_2f5e98fb7dfb
#define XTD_xtd_string_44929f79_d6b5_452e_9c17_2f5e98fb7dfb

#include <xtd/optional>
#include <algorithm>
#include <iosfwd>
#include <iterator>
#include <limits>
#include <string>

namespace xtd
{

/// Implements the string_view proposal N3609.
template<class CharT, class Traits = std::char_traits<CharT>>
class basic_string_view
{
public:
	/// \name Member types
	//@{
	
	using traits_type = Traits;
	using value_type = CharT;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = CharT&;
	using const_reference = const CharT&;
	using pointer = CharT*;
	using const_pointer = const CharT*;

	using iterator = const CharT*; // basic_string_view is non-modifying
	using const_iterator = iterator;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	
	//@}
	/// \name Construction & Assignment
	//@{
	
	/// Construct an empty string_view.
	constexpr basic_string_view() noexcept
		: _data{nullptr}, _len{0}
	{ }
	/// Construct from a null-terminated C string.
	basic_string_view(const CharT* chars) noexcept(noexcept(Traits::length(chars)))
		: _data{chars}, _len{chars ? Traits::length(chars) : 0}
	{ }
	/// Construct from the half-open range of characters `[first, last)`
	basic_string_view(const CharT* first, const CharT* last)
		: _data{first}, _len{static_cast<size_type>(last - first)}
	{ }
	/// Construct from C-string taking the first \p len character only.
	constexpr basic_string_view(const CharT* chars, size_type len) noexcept
		: _data{chars}, _len{chars ? len : 0}
	{ }
	/// Construct from a std::basic_string
	template<class Allocator>
	basic_string_view(const std::basic_string<CharT, Traits, Allocator>& str) noexcept
		: _data{str.data()}, _len{str.size()}
	{ }
	/// Construct from a character initializer list.
	constexpr basic_string_view(std::initializer_list<CharT> chars) noexcept
		: _data{chars.begin()}, _len{chars.size()}
	{ }
	
	constexpr basic_string_view(const basic_string_view& s) = default;
	constexpr basic_string_view(basic_string_view&& s) = default;
	
	basic_string_view& operator = (const basic_string_view& s) = default;
	basic_string_view& operator = (basic_string_view&& s) = default;
	
	//@}
	/// \name Iterators
	//@{
	
	/// Returns an iterator to the beginning
	constexpr const_iterator begin() const noexcept { return cbegin(); }
	/// Returns an iterator to the end
	constexpr const_iterator end() const noexcept { return cend(); }
	/// Returns an iterator to the beginning
	constexpr const_iterator cbegin() const noexcept { return data(); }
	/// Returns an iterator to the end
	constexpr const_iterator cend() const noexcept { return data() + size(); }
	
	/// Returns a reverse iterator to the beginning
	const_reverse_iterator rbegin() const noexcept { return crbegin(); }
	/// Returns a reverse iterator to the end
	const_reverse_iterator rend() const noexcept { return crend(); }
	/// Returns a reverse iterator to the beginning
	const_reverse_iterator crbegin() const noexcept { return reverse_iterator{cend()}; }
	/// Returns a reverse iterator to the end
	const_reverse_iterator crend() const noexcept { return reverse_iterator{cbegin()}; }

	//@}
	/// \name Capacity
	//@{
	
	/// Returns the number of characters
	constexpr size_type size() const noexcept { return _len; }
	/// Returns the number of characters
	constexpr size_type length() const noexcept { return size(); }
	/// Checks whether the string is empty
	constexpr bool empty() const noexcept { return size() == 0; }

	//@}
	/// \name Element access
	//@{

	/// Get a reference to the character at the specified position.
	constexpr const CharT& operator[](size_type pos) const noexcept
	{
		return assertNonNull(), assertNonEmpty(), _data[pos];
	}
	/**
	 Get a reference to the character at the specified position.
	 
	 \throws std::out_of_range if <tt>pos >= size()</tt>.
	 */
	constexpr const CharT& at(size_type pos) const
	{
		return assertNonNull(), checkPosition(pos), (*this)[pos];
	}
	/// Get a reference to the first character.
	constexpr const CharT& front() const noexcept
	{
		return (*this)[0];
	}
	/// Get a reference to the last character.
	constexpr const CharT& back() const noexcept
	{
		return (*this)[size() - 1];
	}
	/// Get the underlying data pointer, which may be NULL.
	constexpr const CharT* data() const noexcept
	{
		return _data;
	}

	//@}
	/// \name Modifiers
	//@{
	
	/// Reset to an empty string_view.
	void clear() noexcept
	{
		*this = basic_string_view{};
	}
	/// Remove the leading \p n characters, undefined behavior if <tt>n > size()</tt>.
	void remove_prefix(size_type n)
	{
		assert(n <= size() && "xtd::basic_string_view::remove_prefix: n is bigger than string");
		_data += n;
		_len -= n;
	}
	/// Remove the trailing \p n characters, undefined behavior if <tt>n > size()</tt>.
	void remove_suffix(size_type n)
	{
		assert(n <= size() && "xtd::basic_string_view::remove_suffix: n is bigger than string");
		_len -= n;
	}
	/// Swap with another string_view object.
	void swap(basic_string_view& other) noexcept
	{
		std::swap(_data, other._data);
		std::swap(_len, other._len);
	}

	//@}
	/// \name String Operations
	//@{

	/**
	 Get string_view for the specified substring range.
	 
	 \param pos The starting position of the substring, must be < size().
	 \param n Optional length of the substring, truncated to the remaining length if required. If empty takes the remaining string length.
	 
	 \throws std::out_of_range if <tt>pos >= size()</tt>.
	 */
		constexpr basic_string_view substr(size_type pos, optional<size_type> n = nullopt) const
	{
		return checkPosition(pos), basic_string_view{offset(pos), tail(pos, n)};
	}
	/**
	 Compare contents with another string_view objects.
	 
	 \return Returns 0 if the two string match exactly, otherwise -1 if \p this compares lexicographically less than \p other or +1 otherwise.
	 */
	int compare(basic_string_view x) const
	{
		auto len = std::min(size(), x.size()); // Prevent NULL derefence in empty strings
		if(len <= 0)
			return size() == x.size() ? 0 : (size() < x.size() ? -1 : 1);
		
		auto cmp = Traits::compare(data(), x.data(), _len);
		return cmp != 0 ? cmp : (size() == x.size() ? 0 : (size() < x.size() ? -1 : 1));
	}
	/// True if \p this starts with exactly the same string as \p prefix.
	bool starts_with(basic_string_view prefix) const
	{
		assertNonNull();
		prefix.assertNonNull();
		return size() >= prefix.size() && Traits::compare(data(), prefix.data(), prefix.size()) == 0;
	}
	/// True if <tt>!empty()</tt> and the first character of \p this is P ch.
	bool starts_with(CharT ch) const
	{
		return size() > 0 && Traits::eq(ch, front());
	}
	/// True if \p this ends with exactly the same string as \p suffix.
	bool ends_with(basic_string_view suffix) const
	{
		assertNonNull();
		suffix.assertNonNull();
		return size() >= suffix.size() && Traits::compare(data() + size() - suffix.size(), suffix.data(), suffix.size()) == 0;
	}
	/// True if <tt>!empty()</tt> and the last character of \p this is P ch.
	bool ends_with(CharT ch) const
	{
		return size() > 0 && Traits::eq(ch, back());
	}
	/// Return a std::basic_string with the contents specified by \p this.
	template<class Allocator>
	explicit operator std::basic_string<CharT, Traits, Allocator> () const
	{
		using str = std::basic_string<CharT, Traits, Allocator>;
		return _data ? str{_data, _len} : str{};
	}
	
	//@}
	/// \name Searching
	/// All methods in this group return an empty optional if the search fails, otherwise it contains the index of the first character found.
	//@{

	optional<size_type> find(basic_string_view s) const
	{
		assertNonNull();
		return forwardIterOffset(std::search(begin(), end(), s.begin(), s.end(), Traits::eq));
	}
	optional<size_type> find(CharT ch) const
	{
		assertNonNull();
		return forwardIterOffset(std::find_if(begin(), end(), [ch] (CharT x) { return Traits::eq(x, ch); }));
	}
	optional<size_type> rfind(basic_string_view s) const
	{
		assertNonNull();
		return forwardIterOffset(std::find_end(begin(), end(), s.begin(), s.end(), Traits::eq));
	}
	optional<size_type> rfind(CharT ch) const
	{
		assertNonNull();
		return reverseIterOffset(std::find_if(rbegin(), rend(), [ch] (CharT x) { return Traits::eq(x, ch); }));
	}
	optional<size_type> find_first_of(basic_string_view s) const
	{
		assertNonNull();
		return forwardIterOffset(std::find_first_of(begin(), end(), s.begin(), s.end(), Traits::eq));
	}
	optional<size_type> find_first_of(CharT ch) const
	{
		return find(ch);
	}
	optional<size_type> find_last_of(basic_string_view s) const
	{
		assertNonNull();
		return reverseIterOffset(std::find_first_of(rbegin(), rend(), s.begin(), s.end(), Traits::eq));
	}
	optional<size_type> find_last_of(CharT ch) const
	{
		return rfind(ch);
	}
	optional<size_type> find_first_not_of(basic_string_view s) const
	{
		assertNonNull();
		return forwardIterOffset(findNotOf(begin(), end(), s));
	}
	optional<size_type> find_first_not_of(CharT ch) const
	{
		assertNonNull();
		return forwardIterOffset(std::find_if_not(begin(), end(), [ch] (CharT x) { return Traits::eq(x, ch); }));
	}
	optional<size_type> find_last_not_of(basic_string_view s) const
	{
		return reverseIterOffset(findNotOf(rbegin(), rend(), s));
	}
	optional<size_type> find_last_not_of(CharT ch) const
	{
		assertNonNull();
		return reverseIterOffset(std::find_if_not(rbegin(), rend(), [ch] (CharT x) { return Traits::eq(ch, x); }));
	}

	//@}
	
private:
	// Get the pointer _data + min(pos, size())
	constexpr const CharT* offset(size_type pos) const noexcept
	{
		return _data + (pos >= size() ? size() : pos);
	}
	// Get the length of a substring starting at pos with size n
	constexpr size_type tail(size_type pos, optional<size_type> n) const noexcept
	{
		return !n || *n + pos > size() ? size() - pos : *n;
	}
	constexpr bool assertNonNull() const noexcept
	{
		return assert(_data && "xtd::basic_string_view points to NULL."), true;
	}
	constexpr bool assertNonEmpty() const noexcept
	{
		return assert(_len > 0 && "xtd::basic_string_view has length zero."), true;
	}
	constexpr bool checkPosition(size_type pos) const
	{
		return pos >= size() ? throw std::out_of_range{"xtd::basic_string_view pos out of range."} : true;
	}
	optional<size_type> forwardIterOffset(iterator it) const
	{
		return it == end() ? optional<size_type>{nullopt} : optional<size_type>{it - begin()};
	}
	optional<size_type> reverseIterOffset(reverse_iterator it) const
	{
		return it == rend() ? optional<size_type>{nullopt} : optional<size_type>{it.base() - 1 - begin()};
	}
	template<class Iter>
	static Iter findNotOf(Iter first, Iter last, basic_string_view s)
	{
		while(first++ != last)
		{
			if(Traits::find(s.data(), s.size(), *first) == nullptr)
				return first;
		}
		return last;
	}
	
	const CharT* _data;
	size_type _len;
};

/// \name Relational operators
/// \relates xtd::basic_string_view
//@{

/// True if the two strings are lexicographically equal.
template<class CharT, class Traits>
inline bool operator == (basic_string_view<CharT, Traits> a, basic_string_view<CharT, Traits> b)
{
	return a.size() == b.size() && a.compare(b) == 0;
}

/// True if the two strings are lexicographically equal.
template<class CharT, class Traits>
inline bool operator == (const CharT* a, basic_string_view<CharT, Traits> b)
{
	return basic_string_view<CharT, Traits>{a} == b;
}

/// True if the two strings are lexicographically equal.
template<class CharT, class Traits>
inline bool operator == (basic_string_view<CharT, Traits> a, const CharT* b)
{
	return a == basic_string_view<CharT, Traits>{b};
}

/// True if the two strings are lexicographically inequal.
template<class CharT, class Traits>
inline bool operator != (basic_string_view<CharT, Traits> a, basic_string_view<CharT, Traits> b)
{
	return a.size() != b.size() || a.compare(b) != 0;
}

/// True if the two strings are lexicographically inequal.
template<class CharT, class Traits>
inline bool operator != (const CharT* a, basic_string_view<CharT, Traits> b)
{
	return basic_string_view<CharT, Traits>{a} != b;
}

/// True if the two strings are lexicographically inequal.
template<class CharT, class Traits>
inline bool operator != (basic_string_view<CharT, Traits> a, const CharT* b)
{
	return a != basic_string_view<CharT, Traits>{b};
}

/// True if \p a compares lexicographically less than \p b.
template<class CharT, class Traits>
inline bool operator < (basic_string_view<CharT, Traits> a, basic_string_view<CharT, Traits> b)
{
	return a.compare(b) < 0;
}

/// True if \p a compares lexicographically less than \p b.
template<class CharT, class Traits>
inline bool operator < (const CharT* a, basic_string_view<CharT, Traits> b)
{
	return basic_string_view<CharT, Traits>{a} < b;
}

/// True if \p a compares lexicographically less than \p b.
template<class CharT, class Traits>
inline bool operator < (basic_string_view<CharT, Traits> a, const CharT* b)
{
	return a < basic_string_view<CharT, Traits>{b};
}

/// True if \p a compares lexicographically greater than \p b.
template<class CharT, class Traits>
inline bool operator > (basic_string_view<CharT, Traits> a, basic_string_view<CharT, Traits> b)
{
	return a.compare(b) > 0;
}

/// True if \p a compares lexicographically greater than \p b.
template<class CharT, class Traits>
inline bool operator > (const CharT* a, basic_string_view<CharT, Traits> b)
{
	return basic_string_view<CharT, Traits>{a} > b;
}

/// True if \p a compares lexicographically greater than \p b.
template<class CharT, class Traits>
inline bool operator > (basic_string_view<CharT, Traits> a, const CharT* b)
{
	return a > basic_string_view<CharT, Traits>{b};
}

/// True if \p a compares lexicographically less than or equal to \p b.
template<class CharT, class Traits>
inline bool operator <= (basic_string_view<CharT, Traits> a, basic_string_view<CharT, Traits> b)
{
	return a.compare(b) <= 0;
}

/// True if \p a compares lexicographically less than or equal to \p b.
template<class CharT, class Traits>
inline bool operator <= (const CharT* a, basic_string_view<CharT, Traits> b)
{
	return basic_string_view<CharT, Traits>{a} <= b;
}

/// True if \p a compares lexicographically less than or equal to \p b.
template<class CharT, class Traits>
inline bool operator <= (basic_string_view<CharT, Traits> a, const CharT* b)
{
	return a <= basic_string_view<CharT, Traits>{b};
}

/// True if \p a compares lexicographically greater than or equal to \p b.
template<class CharT, class Traits>
inline bool operator >= (basic_string_view<CharT, Traits> a, basic_string_view<CharT, Traits> b)
{
	return a.compare(b) >= 0;
}

/// True if \p a compares lexicographically greater than or equal to \p b.
template<class CharT, class Traits>
inline bool operator >= (const CharT* a, basic_string_view<CharT, Traits> b)
{
	return basic_string_view<CharT, Traits>{a} >= b;
}

/// True if \p a compares lexicographically greater than or equal to \p b.
template<class CharT, class Traits>
inline bool operator >= (basic_string_view<CharT, Traits> a, const CharT* b)
{
	return a >= basic_string_view<CharT, Traits>{b};
}

//@}
/// \name Swap
/// \relates xtd::basic_string_view
//@{

/// Swap the referenced contents of two basic_string_view objects.
template<class CharT, class Traits>
inline void swap(basic_string_view<CharT, Traits>& a, basic_string_view<CharT, Traits>& b) noexcept
{
	a.swap(b);
}

//@}
/// \name Specialized algorithms
/// \relates xtd::basic_string_view
//@{

/// convert a basci-string_view to a std::string_view, optionally specifying a custom allocator.
template<class CharT, class Traits, class Allocator = std::allocator<CharT>>
inline std::basic_string<CharT, Traits, Allocator> to_string(basic_string_view<CharT, Traits> s, const Allocator& alloc = Allocator{})
{
	return std::basic_string<CharT, Traits, Allocator>{s.cbegin(), s.cend(), alloc};
}

//@}

// TODO: add numeric conversions
// TODO: std::hash<...>

/// \name Inserter
/// \relates xtd::basic_string_view
//@{

/// Performs a formatted output of the content referenced by \p s according to the rules of FormattedOuput [ostream.formatted.reqmts].
template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator << (std::basic_ostream<CharT, Traits>& os, basic_string_view<CharT, Traits> s)
{
	using Stream = std::basic_ostream<CharT, Traits>;
	typename Stream::sentry ok{os};
	if(ok)
	{
		try
		{
			auto width = os.width();
			if(width > s.size())
			{
				auto adjustfield = os.flags() & Stream::adjustfield;
				if(adjustfield == Stream::left)
				{
					os.rdbuf()->sputn(s.data(), s.size());
					std::fill_n(std::ostreambuf_iterator<CharT>{os}, width - s.size(), os.fill());
				}
				else
				{
					std::fill_n(std::ostreambuf_iterator<CharT>{os}, width - s.size(), os.fill());
					os.rdbuf()->sputn(s.data(), s.size());
				}
			}
			else
				os.rdbuf()->sputn(s.data(), s.size());
			os.width(0);
		}
		catch(...)
		{
			os.setstate(Stream::badbit);
			throw;
		}
	}
	else
		os.setstate(Stream::failbit);
	return os;
}

//@}
/// \name basic_string_view specializations
/// \relates xtd::basic_string_view
//@{

/// Specialization of basic_string_view for \p char.
using string_view = basic_string_view<char>;
/// Specialization of basic_string_view for \p char16_t.
using u16string_view = basic_string_view<char16_t>;
/// Specialization of basic_string_view for \p char32_t.
using u32string_view = basic_string_view<char32_t>;
/// Specialization of basic_string_view for \p wchar_t.
using wstring_view = basic_string_view<wchar_t>;

//@}

} // namespace xtd

#endif // XTD_xtd_string_44929f79_d6b5_452e_9c17_2f5e98fb7dfb