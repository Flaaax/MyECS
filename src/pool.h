#pragma once
#include<unordered_map>
#include<stdexcept>
#include<optional>
#include"types.h"


namespace myecs {

	namespace pool {
	#pragma warning(push)
	#pragma warning(disable:26495)
		template<class Base, size_t Size>
		class ClassData {
		private:
			unsigned char data[Size];
			void (*move_construct)(void* self_data, void* other_data) = nullptr;

		public:
			ClassData() {}
			ClassData(ClassData&& other)noexcept :move_construct(other.move_construct) {
				if (move_construct) {
					move_construct(data, other.data);
				}
			}
			~ClassData() {
				destroy();
			}

			Base* get() {
				return reinterpret_cast<Base*>(data);
			}

			const Base* get()const {
				return reinterpret_cast<const Base*>(data);
			}

			template<class T>
				requires std::derived_from<T, Base>
			T* get() {
				return reinterpret_cast<T*>(data);
			}

			template<class T>
				requires std::derived_from<T, Base>
			const T* get()const {
				return reinterpret_cast<const T*>(data);
			}

			void destroy() {
				if (has_value()) {
					move_construct = nullptr;
					get()->~Base();
				}
			}

			template<class T, class ...Args>
				requires std::derived_from<T, Base>
			void emplace(Args&&... args) {
				destroy();
				new (data) T(std::forward<Args>(args)...);
				move_construct = [](void* self_data, void* other_data) {
					new (self_data) T(std::move(*reinterpret_cast<T*>(other_data)));
				};
			}

			bool has_value()const {
				return (bool)(move_construct);
			}
		};
	#pragma warning(pop)

		class IPool {
		public:
			virtual ~IPool() {}
			virtual size_t count()const = 0;
			virtual size_t max_count()const = 0;
		};

		template<class T>
		class Pool :public IPool {
		private:
			std::vector<std::optional<T>> storage;
			IdGen<size_t> ids;

		public:
			Pool() {}
			Pool(Pool&& other)noexcept :
				storage(std::move(other.storage)),
				ids(std::move(other.ids)) {
			}

			template<class ...Args>
			size_t create(Args&&... args) {
				if (ids.full()) {
					storage.emplace_back();
				}
				size_t id = ids.get();
				storage[id].emplace(std::forward<Args>(args)...);
				return id;
			}

			bool valid(size_t id)const {
				return ids.active(id);
			}

			T& get(size_t id) {
				return storage[id].value();
			}

			T* try_get(size_t id) {
				if (valid(id)) {
					return &get(id);
				}
				return nullptr;
			}

			void destroy(size_t id) {
				storage[id].reset();
				ids.ret(id);
			}

			size_t count()const override {
				return ids.count();
			}

			size_t max_count()const override {
				return ids.max_count();
			}

			void clear() {
				storage.clear();
				ids.clear();
			}
		};

		//testing...
		class ComponentMgr {
		private:
			using PoolData = ClassData<IPool, sizeof(Pool<int>)>;
			std::unordered_map<size_t, PoolData> pools;

			template<class T>
			Pool<T>* get_pool() {
				PoolData& data = pools[types::type_hash<T>()];
				if (!data.has_value()) {
					data.emplace<Pool<T>>();
				}
				return data.get<Pool<T>>();
			}

			template<class T>
			Pool<T>* try_get_pool() {
				if (auto it = pools.find(types::type_hash<T>()); it != pools.end()) {
					return it->second.get<Pool<T>>();
				}
				return nullptr;
			}

			template<class T>
			const Pool<T>* try_get_pool()const {
				if (auto it = pools.find(types::type_hash<T>()); it != pools.end()) {
					return it->second.get<Pool<T>>();
				}
				return nullptr;
			}

		public:
			ComponentMgr() {}
			ComponentMgr(ComponentMgr&& other)noexcept :pools(std::move(other.pools)) {}

			template<class T, class ...Args>
			size_t emplace(Args&&... args) {
				return get_pool<T>()->create(std::forward<Args>(args)...);
			}

			template<class T>
			T& get(size_t c) {
				return try_get_pool<T>()->get(c);
			}

			template<class T>
			T* try_get(size_t c) {
				if (auto p = try_get_pool<T>()) {
					return p->try_get(c);
				}
				return nullptr;
			}

			template<class T>
			void destroy(size_t c) {
				try_get_pool<T>()->destroy(c);
			}

			template<class T>
			bool valid(size_t c) const {
				if (auto p = try_get_pool<T>()) {
					return p->valid(c);
				}
				return false;
			}
		};
	}//namespace pool

}//namespace myecs