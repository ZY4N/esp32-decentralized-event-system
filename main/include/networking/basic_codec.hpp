#pragma once

#include "util/uix.hpp"
#include <tuple>
#include <array>
#include <system_error>
#include <cstring>
#include <algorithm>
#include "util/reverse_memcpy.hpp"

namespace basic_codec_detail {

template<typename T>
struct codec {

	static constexpr auto buffer_size = ztu::usize{};

	inline static void serialize(const T& value, ztu::u8* &it) {
		T::____unimplemented_type;
	}

	inline static void deserialize(const ztu::u8* &it, T& value) {
		T::____unimplemented_type;
	}
};


template<std::integral I>
struct codec<I> {
	
	static constexpr auto buffer_size = sizeof(I);

	inline static void serialize(I value, ztu::u8* &it) {
		#ifndef __BIG_ENDIAN__
		reverse_memcpy(it, &value, buffer_size);
		#else
		std::memcpy(it, &value, buffer_size);
		#endif
		it += buffer_size;
	}

	inline static void deserialize(const ztu::u8* &it, I& value) {
		#ifndef __BIG_ENDIAN__
		reverse_memcpy(&value, it, buffer_size);
		#else
		std::memcpy(&value, it, buffer_size);
		#endif
		it += buffer_size;
	}
};


template<typename T, ztu::usize Size>
struct codec<std::array<T, Size>> {

	static constexpr auto buffer_size = Size * codec<T>::buffer_size;

	inline static void serialize(const std::array<T, Size>& values, ztu::u8* &it) {
		for (const auto& value : values) {
			codec<T>::serialize(value, it);
		}
	}

	inline static void deserialize(const ztu::u8* &it, T& values) {
		for (const auto& value : values) {
			codec<T>::deserialize(it, value);
		}
	}
};

template<ztu::usize Size>
struct codec<std::array<ztu::u8, Size>> {

	static constexpr auto buffer_size = Size * sizeof(ztu::u8);

	inline static void serialize(const std::array<ztu::u8, Size>& values, ztu::u8* &it) {
		std::copy(values.begin(), values.end(), it);
		it += buffer_size;
	}

	inline static void deserialize(const ztu::u8* &it, std::array<ztu::u8, Size>& values) {
		std::copy_n(it, Size, values.begin());
		it += Size;
	}
};

template<typename... Ts>
struct codec<std::tuple<Ts...>> {

	static constexpr auto buffer_size = ztu::usize{} + (codec<Ts>::buffer_size + ...);

	inline static void serialize(const std::tuple<Ts...>& tuple, ztu::u8* &it) {
		std::apply([&](const auto&... values) {
			([&]<typename T>(const T& value) {
				codec<T>::serialize(value, it);
			}(values), ...);
		}, tuple);
	}

	inline static void deserialize(const ztu::u8* &it, std::tuple<Ts...>& tuple) {
		std::apply([&](auto&... values) {
			([&]<typename T>(T& value) {
				codec<T>::deserialize(it, value);
			}(values), ...);
		}, tuple);
	}
};

} // basic_codec_detail

namespace basic_codec {

template<typename T>
constexpr auto serialized_size_v = basic_codec_detail::codec<T>::buffer_size;

template<typename T>
using buffer_t = std::array<ztu::u8, serialized_size_v<T>>;

template<typename T>
inline auto serialize(const T& value) {
	buffer_t<T> buffer;

	auto it = buffer.data();
	basic_codec_detail::codec<T>::serialize(value, it);

	return buffer;
}

template<typename T>
inline T deserialize(const T& buffer) {
	T value{};

	auto it = buffer.data();
	basic_codec_detail::codec<T>::deserialize(it, value);

	return value;
}

template<typename T>
struct based_on {
	using T::___wrong_parameters;
};

template<typename... Ts>
struct based_on<std::tuple<Ts...>> {

	using m_tuple_t = std::tuple<Ts...>;
	using m_buffer_t = buffer_t<m_tuple_t>; 

	inline static auto deserialize(const m_buffer_t& buffer, Ts&... values) {
		m_tuple_t value{};

		auto it = buffer.data();
		(basic_codec_detail::codec<Ts>::deserialize(it, values), ...);

		return value;
	}

	inline static auto serialize(const Ts&... values) {
		m_buffer_t buffer;
		
		auto it = buffer.data();
		(basic_codec_detail::codec<Ts>::serialize(values, it), ...);

		return buffer;
	}

	template<ztu::usize I>
	inline static auto serialize_at(const std::tuple_element_t<I, m_tuple_t>& value, m_buffer_t& buffer) {

		const auto offset = [&]<ztu::usize... Is>(std::index_sequence<Is...>) {
			return ztu::usize{} + (
				serialized_size_v<std::tuple_element_t<Is, m_tuple_t>> + ...
			);
		}(std::make_index_sequence<I>());
		
		auto it = buffer.data() + offset;
		basic_codec_detail::codec<std::tuple_element_t<I, m_tuple_t>>::serialize(value, it);

		return buffer;
	}

};

} // basic_codec
