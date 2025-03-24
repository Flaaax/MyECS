#pragma once
#ifndef MYECS_ENTITY_H
#define MYECS_ENTITY_H
#include"component.h"
#include"dense_map.h"
//#include<memory_resource>


namespace myecs {

	//single thread only
	//support move construct for components
	//better use only one instance per program (but you dont have to)
	class Registry {
	private:
		using component = IComponentPool::component;
		using component_type = id_type;
		using archetype_view = IComponentPool::archetype_view;
		using ComponentPoolData = pool::ClassData<IComponentPool, sizeof(ComponentPool<int>)>;

		class _ComponentRegistry {
		private:
			inline static id_type component_id_reg = 0;
		public:
			template<class T>
			static id_type getComponentId() {
				static id_type _id = component_id_reg++;
				return _id;
			}

		};

		std::vector<ComponentPoolData> pools;
		IdGen<entity> ids;
		std::vector<SparseSet<id_type>> entity_components;

		template<class T>
		ComponentPool<T>& get_pool() {
			id_type component_id = _ComponentRegistry::getComponentId<T>();
			if (pools.size() <= component_id) {
				pools.resize(component_id + 1);
			}
			ComponentPoolData& data = pools[component_id];
			if (!data.has_value()) {
				data.emplace<ComponentPool<T>>();
			}
			return *data.get<ComponentPool<T>>();
		}

		template<class T>
		ComponentPool<T>* try_get_pool() {
			id_type component_id = _ComponentRegistry::getComponentId<T>();
			if (pools.size() <= component_id) {
				return nullptr;
			}
			auto& data = pools[component_id];
			if (!data.has_value()) {
				return nullptr;
			}
			return data.get<ComponentPool<T>>();
		}

		template<class T>
		const ComponentPool<T>* try_get_pool()const {
			return const_cast<Registry*>(this)->try_get_pool<T>();
		}

		static IntVector<entity> get_common(std::initializer_list<const SparseSet<entity>*> archetypes) {
			size_t min_size = SparseSet<entity>::_max_size;
			const SparseSet<entity>* minSet = nullptr;
			//std::cout << "archetype count: " << archetypes.size() << "\n";
			for (auto* archetype : archetypes) {
				//std::cout << "archetype size:" << archetype->size() << std::endl;
				if (archetype->size() < min_size) {
					min_size = archetype->size();
					minSet = archetype;
					//std::cout << "find option archetype, size=" << min_size << std::endl;
				}
			}
			if (!minSet) {
				return {};
			}
			IntVector<entity> ret;
			for (auto e : *minSet) {
				bool ok = true;
				for (auto* a : archetypes) {
					if (a == minSet) continue;
					if (!a->has(e)) {
						ok = false;
						break;
					}
				}
				if (ok) {
					ret.emplace_back(e);
				}
			}
			return ret;
		}

		Registry(const Registry&) = delete;

	public:
		explicit Registry() = default;
		Registry(Registry&& other) noexcept :
			pools(std::move(other.pools)),
			ids(std::move(other.ids)),
			entity_components(std::move(other.entity_components)) {
		}

		//warning: when you emplace new component, the reference may expire!
		template<class T, class ...Args>
		T& emplace(entity e, Args&&... args) {
			if constexpr (myecs_debug_level) {
				if (!ids.active(e)) {
					throw std::runtime_error("invalid entity");
				}
			}
			size_t id = e.get_id();
			if (entity_components.size() <= id) {
				entity_components.resize(id + 1);
			}
			entity_components[id].insert(_ComponentRegistry::getComponentId<T>());
			ComponentPool<T>& pool = get_pool<T>();
			return pool.create(e, std::forward<Args>(args)...);
		}

		template<class T, class ...Args>
		T& get_or_emplace(entity e, Args&&... args) {
			ComponentPool<T>& pool = get_pool<T>();
			if (pool.has(e)) {
				return pool.get(e);
			}
			else {
				return emplace<T>(e, std::forward<Args>(args)...);
			}
		}

		template<class ...Types>
		decltype(auto) emplace_all(entity e, const Types&... types) {
			return std::forward_as_tuple(emplace<Types>(e, types)...);
		}

		template<class T>
		MYECS_NODISCARD bool has(entity e)const {
			if (const ComponentPool<T>* pool = try_get_pool<T>()) {
				return pool->has(e);
			}
			return false;
		}

		template<class ...Types>
			requires (sizeof...(Types) >= 2)
		MYECS_NODISCARD bool has(entity e)const {
			return (has<Types>(e) && ...);
		}

		//warning: when you emplace new component, the reference may expire!
		template<class T>
		MYECS_NODISCARD T& get(entity e) {
			if constexpr (myecs_debug_level) {
				if (!valid(e)) {
					throw std::runtime_error("invalid entity");
				}
			}
			ComponentPool<T>& pool = get_pool<T>();
			return pool.get(e);
		}

		template<class T>
		MYECS_NODISCARD T* try_get(entity e) {
			if (!has<T>(e)) {
				return nullptr;
			}
			return &get<T>(e);
		}

		template<class ...Types>
			requires (sizeof...(Types) >= 2)
		MYECS_NODISCARD decltype(auto) try_get(entity e) {
			return std::make_tuple(try_get<Types>(e)...);
		}

		template<class T>
		void destroy(entity e) {
			if (auto pool = try_get_pool<T>()) {
				pool->destroy(e);
				size_t id = e.get_id();
				if (entity_components.size() <= id) {
					return;
				}
				entity_components[id].erase(_ComponentRegistry::getComponentId<T>());
			}
		}

		template<class ...Types>
			requires (sizeof...(Types) >= 2)
		void destroy(entity e) {
			(destroy<Types>(e), ...);
		}

		template<class ...Types>
			requires (sizeof...(Types) >= 2)
		MYECS_NODISCARD std::tuple<Types&...> get(entity e) {
			return std::forward_as_tuple(get<Types>(e)...);
		}

		MYECS_NODISCARD entity create() {
			return ids.get();
		}

		void destroy(entity e) {
			if (!ids.active(e)) {
				return;
			}
			ids.ret(e);
			size_t id = static_cast<size_t>(e.id);
			if (entity_components.size() <= id) {
				return;
			}
			for (auto cid : entity_components[id]) {
				pools[cid].get()->destroy(e);
			}
			entity_components[id].clear();
		}

		MYECS_NODISCARD bool valid(entity e)const {
			return ids.active(e);
		}

		template<class T>
		MYECS_NODISCARD archetype_view view() {
			return get_pool<T>().view();
		}

		template<class ...Types>
			requires (sizeof...(Types) >= 2)
		MYECS_NODISCARD decltype(auto) view() {
			return get_common({ (&view<Types>())... });
		}

		//clear all the items inside the register
		void reset() {
			ids.clear();
			entity_components.clear();
			for (auto& pool : pools) {
				pool.get()->clear();
			}
		}

		MYECS_NODISCARD size_t entity_count()const {
			return ids.count();
		}

		MYECS_NODISCARD size_t max_entity_count()const {
			return ids.max_count();
		}

		MYECS_NODISCARD size_t component_count()const {
			size_t ret = {};
			for (const auto& pool : pools) {
				ret += pool.get()->count();
			}
			return ret;
		}

		MYECS_NODISCARD size_t max_component_count()const {
			size_t ret = {};
			for (const auto& pool : pools) {
				ret += pool.get()->max_count();
			}
			return ret;
		}
	};

}//namespace myecs


#endif