#pragma once
#ifndef MYECS_COMPONENT_H
#define MYECS_COMPONENT_H
#include"container.h"
#include"pool.h"
#include<format>
#include<functional>
#include<optional>


namespace myecs {
	class IComponentPool {
	public:
		using component = size_t;
		using archetype_view = const SparseSet<entity>&;
	protected:
		//static constexpr component null_component = std::numeric_limits<component>::max();
		SparseSet<entity> archetype;
		IntVector<component> entity_to_component;

	public:
		IComponentPool() = default;
		IComponentPool(IComponentPool&& other) noexcept :
			archetype(std::move(other.archetype)),
			entity_to_component(std::move(other.entity_to_component)) {
		}
		virtual ~IComponentPool() = default;

		virtual void destroy(entity e) = 0;

		MYECS_NODISCARD bool has(entity e)const {
			return archetype.has(e);
		}

		MYECS_NODISCARD archetype_view view()const {
			return archetype;
		}

		virtual void clear() = 0;

		MYECS_NODISCARD virtual size_t count()const = 0;
		MYECS_NODISCARD virtual size_t max_count()const = 0;
	};


	template<class T>
	class ComponentPool :public IComponentPool {
	private:
		pool::Pool<T> pool;

	public:
		ComponentPool() {}
		~ComponentPool() {}

		ComponentPool(ComponentPool&& other)noexcept :
			IComponentPool(std::move(other)),
			pool(std::move(other.pool)) {
		}

		template<class ...Args>
		T& create(entity e, Args&&... args) {
			if (has(e)) {
				throw std::runtime_error("entity already has component");
			}
			component id = pool.create(std::forward<Args>(args)...);
			archetype.insert(e);
			entity_to_component.force_get(e.get_id()) = id;
			return pool.get(id);
		}

		MYECS_NODISCARD T& get(entity e) {
			MYECS_ASSERT(has(e), "invalid entity");
			component c = entity_to_component[e.get_id()];
			return pool.get(c);
		}

		void clear()override {
			pool.clear();
			archetype.clear();
			entity_to_component.clear();
		}

		void destroy(entity e)override {
			if (!has(e))return;
			archetype.erase(e);
			auto c = entity_to_component[e.get_id()];
			pool.destroy(c);
		}

		MYECS_NODISCARD size_t count()const override {
			return pool.count();
		}
		MYECS_NODISCARD size_t max_count()const override {
			return pool.max_count();
		}
	};

}//namespace myecs


#endif