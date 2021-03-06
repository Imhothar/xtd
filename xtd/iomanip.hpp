/*
 Copyright 2014 Miro Knejp
 
 See the accompanied LICENSE file for licensing details.
 */

/**
 \file
 Adds new IO stream manipulators.
 
 \author Miro Knejp
 */

#pragma once

#include <array>
#include <iomanip>
#include <type_traits>
#include <vector>

namespace xtd
{

////////////////////////////////////////////////////////////////////////
// Unformatted Binary Stream Reading and Writing
//

namespace detail {
namespace iomanip
{
	template<class T, class Size>
	struct UnformattedBinaryStreamManipulator
	{
		T* target;
		Size count; // Number of instances of T, not bytes
	};
	
	template<class T, class CharT, class Traits, class Size>
	std::basic_istream<CharT, Traits>& operator>> (std::basic_istream<CharT, Traits>& in, UnformattedBinaryStreamManipulator<T, Size> brm)
	{
		static_assert(sizeof(CharT) == 1, "xtd::operator>>: Unformatted input is only supported for streams with single-byte elements.");
		return in.read(reinterpret_cast<CharT*>(brm.target), (std::is_void<T>::value ? 1 : sizeof(*brm.target)) * brm.count);
	}
	
	template<class T, class CharT, class Traits, class Size>
	std::basic_ostream<CharT, Traits>& operator<< (std::basic_ostream<CharT, Traits>& out, UnformattedBinaryStreamManipulator<T, Size> brm)
	{
		static_assert(sizeof(CharT) == 1, "xtd::operator<<: Unformatted output is only supported for streams with single-byte elements.");
		return out.write(reinterpret_cast<CharT*>(brm.target), (std::is_void<T>::value ? 1 : sizeof(*brm.target)) * brm.count);
	}
	
}} // namespace detail::iomanip

/**
 A manipulator for std::basic_*stream to read/write an object as unformatted binary.
 
 The object type to be read or written must be POD.
 
 \code
 std::ifstream f{"file.bin"};
 POD x;
 f >> xtd::unformatted(x);
 \endcode
 */
template<class T>
auto unformatted(T& target)
	-> detail::iomanip::UnformattedBinaryStreamManipulator<T, std::size_t>
{
	static_assert(std::is_pod<T>::value, "xtd::unformatted: Only POD types can be read/written unformatted to/from a basic_*stream.");
	return {&target, 1};
}

/**
 A manipulator for std::basic_*stream to read/write a contiguous region of objects in memory as unformatted binary.
 
 The object type to be read or written must be POD.
 
 \code
 std::ifstream f{"file.bin"};
 POD* memory = some_buffer();
 f >> xtd::unformatted(memory, some_buffer_num_elements());
 \endcode
 
 If the type resolves to \c void then count is treated as a number of bytes instead
 of elements.
 */
template<class T, class Size>
auto unformatted(T* target, Size count)
	-> detail::iomanip::UnformattedBinaryStreamManipulator<T, Size>
{
	static_assert(std::is_pod<T>::value, "xtd::unformatted: Only POD types can be read/written unformatted to/from a basic_*stream.");
	return {target, count};
}

/**
 A manipulator for std::basic_*stream to read/write an array of objects as unformatted binary.
 
 The object type to be read or written must be POD.
 
 \code
 std::ifstream f{"file.bin"};
 POD x[5];
 f >> xtd::unformatted(x);
 \endcode
 */
template<class T, std::size_t N>
auto unformatted(T (&target)[N])
	-> detail::iomanip::UnformattedBinaryStreamManipulator<T, std::size_t>
{
	static_assert(std::is_pod<T>::value, "xtd::unformatted: Only POD types can be read/written unformatted to/from a basic_*stream.");
	return {target, N};
}

/**
 A manipulator for std::basic_*stream to read/write the content of an std::array as unformatted binary.
 
 The object type to be read or written must be POD.
 
 \code
 std::ifstream f{"file.bin"};
 std::array<POD, 5> arr;
 f >> xtd::unformatted(arr);
 \endcode
 */
template<class T, std::size_t N>
auto unformatted(std::array<T, N>& arr)
	-> detail::iomanip::UnformattedBinaryStreamManipulator<T, std::size_t>
{
	static_assert(std::is_pod<T>::value, "xtd::unformatted: Only POD types can be read/written unformatted to/from a basic_*stream.");
	return {arr.data(), arr.size()};
}

/**
 A manipulator for std::basic_*stream to read/write the content of an std::vector as unformatted binary.
 
 The object type to be read or written must be POD.
 The number of elements to be read or written is taken from the size of the vector.
 
 \code
 std::ifstream f{"file.bin"};
 std::vector<POD> v;
 v.resize(10);
 f >> xtd::unformatted(v);
 \endcode
 */
template<class T, class Allocator>
auto unformatted(std::vector<T, Allocator>& v)
	-> detail::iomanip::UnformattedBinaryStreamManipulator<T, typename std::vector<T, Allocator>::size_type>
{
	static_assert(std::is_pod<T>::value, "xtd::unformatted: Only POD types can be read/written unformatted to/from a basic_*stream.");
	return {v.data(), v.size()};
}

/**
 A manipulator for std::basic_*stream to read/write the content of an std::vector as unformatted binary.
 
 The object type to be read or written must be POD.
 The vector is resized to hold the number of elements passed as second argument.
 
 \code
 std::ifstream f{"file.bin"};
 std::uint32_t size;
 std::vector<POD> v;
 f >> xtd::unformatted(size) >> xtd::unformatted(v, size);
 \endcode
 */
template<class T, class Allocator>
auto unformatted(std::vector<T, Allocator>& v, typename std::vector<T, Allocator>::size_type size)
	-> detail::iomanip::UnformattedBinaryStreamManipulator<T, typename std::vector<T, Allocator>::size_type>
{
	static_assert(std::is_pod<T>::value, "xtd::unformatted: Only POD types can be read/written unformatted to/from a basic_*stream.");
	v.resize(size);
	return {v.data(), v.size()};
}

} // namesapce xtd
