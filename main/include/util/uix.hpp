#pragma once

#include <cstdint>
#include <unistd.h>
#include <limits>


namespace ztu {
	namespace uix {
		using u8	= std::uint8_t;
		using u16	= std::uint16_t;
		using u32	= std::uint32_t;
		using u64	= std::uint64_t;
		using usize	= std::size_t;

		using i8	= std::int8_t;
		using i16	= std::int16_t;
		using i32	= std::int32_t;
		using i64	= std::int64_t;
		using isize	= ssize_t;

		[[maybe_unused]] static constexpr auto U8_MAX		= std::numeric_limits<u8 >::max();
		[[maybe_unused]] static constexpr auto U16_MAX		= std::numeric_limits<u16>::max();
		[[maybe_unused]] static constexpr auto U32_MAX		= std::numeric_limits<u32>::max();
		[[maybe_unused]] static constexpr auto U64_MAX		= std::numeric_limits<u64>::max();
		[[maybe_unused]] static constexpr auto USIZE_MAX	= std::numeric_limits<usize>::max();

		[[maybe_unused]] static constexpr auto I8_MAX		= std::numeric_limits<i8 >::max();
		[[maybe_unused]] static constexpr auto I16_MAX		= std::numeric_limits<i16>::max();
		[[maybe_unused]] static constexpr auto I32_MAX		= std::numeric_limits<i32>::max();
		[[maybe_unused]] static constexpr auto I64_MAX		= std::numeric_limits<i64>::max();
		[[maybe_unused]] static constexpr auto ISIZE_MAX	= std::numeric_limits<isize>::max();

		[[maybe_unused]] static constexpr auto U8_MIN		= std::numeric_limits<u8 >::min();
		[[maybe_unused]] static constexpr auto U16_MIN		= std::numeric_limits<u16>::min();
		[[maybe_unused]] static constexpr auto U32_MIN		= std::numeric_limits<u32>::min();
		[[maybe_unused]] static constexpr auto U64_MIN		= std::numeric_limits<u64>::min();
		[[maybe_unused]] static constexpr auto USIZE_MIN	= std::numeric_limits<usize>::min();

		[[maybe_unused]] static constexpr auto I8_MIN		= std::numeric_limits<i8 >::min();
		[[maybe_unused]] static constexpr auto I16_MIN		= std::numeric_limits<i16>::min();
		[[maybe_unused]] static constexpr auto I32_MIN		= std::numeric_limits<i32>::min();
		[[maybe_unused]] static constexpr auto I64_MIN		= std::numeric_limits<i64>::min();
		[[maybe_unused]] static constexpr auto ISIZE_MIN	= std::numeric_limits<isize>::min();
	}

	using namespace uix;
}
