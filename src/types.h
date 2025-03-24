#pragma once
#ifndef MYECS_TYPES_H
#define MYECS_TYPES_H

#include<limits>
#include<xhash>
#include<string_view>

#define MYECS_NODISCARD [[nodiscard]]

namespace myecs {

	//1-debug, 0-release
#ifdef MYECS_DEBUG_LEVEL
	constexpr int myecs_debug_level = MYECS_DEBUG_LEVEL;
#elif _DEBUG
	constexpr int myecs_debug_level = 1;
#else
	constexpr int myecs_debug_level = 0;
#endif

	using id_type = size_t;

	namespace types {
		using u32 = unsigned __int32;
		using u64 = unsigned __int64;
		constexpr u64 u64_max = ::std::numeric_limits<u64>::max();
	}

	struct entity {
		union {
			types::u64 _entity = 0;
			struct {
				types::u32 id;
				types::u32 version;
			};
		};

		constexpr explicit entity() {};
		constexpr entity(const entity& other) :_entity(other._entity) {}
		constexpr explicit entity(size_t _entity) : _entity(_entity) {}
		constexpr entity(types::u32 id, types::u32 version) : id(id), version(version) {}
		MYECS_NODISCARD constexpr bool operator==(const entity& other)const {
			return _entity == other._entity;
		}
		entity& operator=(const entity& other) {
			_entity = other._entity;
			return *this;
		}
		MYECS_NODISCARD constexpr size_t get_id()const {
			return static_cast<size_t>(id);
		}
	};

	constexpr entity null_entity = std::numeric_limits<entity>::max();

	namespace types {
		template<typename Type>
		MYECS_NODISCARD constexpr std::string_view type_name() noexcept {
			std::string_view pretty_function{ static_cast<const char*>(__FUNCSIG__) };
			auto first = pretty_function.find_first_not_of(' ', pretty_function.find_first_of('<') + 1);
			auto value = pretty_function.substr(first, pretty_function.find_last_of('>') - first);
			return value;
		}

		template<typename Type>
		MYECS_NODISCARD constexpr size_t type_hash()noexcept {
			constexpr std::string_view name = type_name<Type>();
			uint64_t hash = 0xCBF29CE484222325; // 64位初始值
			for (auto c : name) {
				hash ^= c;
				hash *= 0x100000001B3; // 64位质数
			}
			return hash;
		}

		namespace internal {
			struct _Register {
			private:
				inline static size_t count = 0;
			public:
				template<class Type>
				MYECS_NODISCARD static size_t type_identifier()noexcept {
					static size_t id = _Register::count++;
					return id;
				}
			};
		}

		template<class Type>
		MYECS_NODISCARD static size_t type_identifier()noexcept {
			return internal::_Register::type_identifier<Type>();
		}
	}//namespace types

}//namespace myecs

namespace std {
	template <>
	struct hash<myecs::entity> {
		size_t operator()(const myecs::entity& e) const noexcept {
			return hash<decltype(myecs::entity::_entity)>()(e._entity);
		}
	};
}//namespace std

#endif