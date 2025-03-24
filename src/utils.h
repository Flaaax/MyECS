#pragma once
#ifndef MYECS_UTILS_H
#define MYECS_UTILS_H
#include<type_traits>
#include<format>
#include<source_location>

#undef max
#undef min


namespace myecs {
	//stolen from entt
	[[nodiscard]] constexpr size_t fast_mod(size_t value, const std::size_t mod)noexcept {
		return value & (mod - 1u);
	}
}


#ifdef _DEBUG
#include<iostream>
namespace myecs {
	namespace internal {
		inline static void __MyAssert(bool true_when_ok, const char* msg, std::source_location location = std::source_location::current()) {
			if (!true_when_ok) {
				auto _msg = std::format("Assertion failed\nfile: {}\nline: {}\nfunction: {}\nmsg: {}\n",
										location.file_name(), location.line(), location.function_name(), msg);
				std::cerr << _msg << std::endl;;
				std::cerr << *reinterpret_cast<int*>(0);
			}
		}
	}
}
#define MYECS_ASSERT(true_when_ok, msg) ::myecs::internal::__MyAssert((true_when_ok), (msg))
#else
#define MYECS_ASSERT(true_when_ok, msg) void(0)
#endif

#endif