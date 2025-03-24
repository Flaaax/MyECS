#pragma once
#ifndef CONTAINER_H
#define CONTAINER_H
#include<vector>
#include<limits>
#include<assert.h>
#include"types.h"
#include"utils.h"
#include<concepts>



namespace myecs {
	template<class T>
	class clever_vector;

	namespace internal {
		struct bool_dummy {
			bool data;

			operator bool& () {
				return data;
			};

			operator const bool& () {
				return data;
			}
		};
	};

	template<class T>
		requires (!std::is_same_v<T, bool>)
	class clever_vector<T> :public std::vector<T> {
		using Super = std::vector<T>;
	public:

		template<class U>
			requires std::is_integral_v<U>
		T& operator[](U index) {
			return Super::operator[](static_cast<size_t>(index));
		}

		T& operator[](entity e) {
			return this->operator[](e.id);
		}

		template<class U>
			requires std::is_integral_v<U>
		const T& operator[](U index)const {
			return Super::operator[](static_cast<size_t>(index));
		}

		const T& operator[](entity e)const {
			return this->operator[](e.id);
		}
	};

	//template<>
	//class clever_vector<bool> :public clever_vector<internal::bool_dummy> {};

	template<class T>
	class IntVector {
	private:
		std::vector<T> data;
		size_t m_size = 0;
	public:
		using const_iterator = const T*;

		IntVector() = default;
		IntVector(const IntVector&) = default;
		IntVector(IntVector&& other)noexcept :data(std::move(other.data)), m_size(other.m_size) {
			other.m_size = 0;
		}
		~IntVector() = default;

		void emplace_back(T t) {
			if (data.size() > m_size) {
				data[m_size] = t;
			}
			else {
				data.emplace_back(t);
			}
			++m_size;
		}

		void push_back(T t) {
			emplace_back(t);
		}

		void pop_back() {
			if (m_size > 0) {
				--m_size;
			}
		}

		void clear() {
			m_size = 0;
		}

		void clear_all() {
			m_size = 0;
			data.clear();
		}

		void shrink() {
			data.resize(m_size);
		}

		T& operator[](size_t i) {
			return data[i];
		}

		const T& operator[](size_t i)const {
			return data[i];
		}

		T& force_get(size_t i) {
			m_size = std::max(i + 1, m_size);
			if (m_size > data.size()) {
				data.resize(m_size, static_cast<T>(0));
			}
			return data[i];
		}

		size_t size()const {
			return m_size;
		}

		const_iterator begin()const {
			return (const_iterator)(data.data());
		}

		const_iterator end()const {
			return (const_iterator)(data.data() + m_size);
		}

		T& front() {
			return data[0];
		}

		const T front()const {
			return data[0];
		}

		T& back() {
			return data[m_size - 1];
		}

		const T back()const {
			return data[m_size - 1];
		}

		void resize(size_t new_size) {
			m_size = new_size;
			if (data.size() < m_size) {
				data.resize(m_size);
			}
		}

		void resize(size_t new_size, T val) {
			if (data.size() < new_size) {
				data.resize(new_size, val);
			}
			for (size_t i = m_size; i < new_size; i++) {
				data[i] = val;
			}
			m_size = new_size;
		}

		bool empty()const {
			return m_size == 0;
		}
	};


	template<class T>
	class IntStack {
	private:
		IntVector<T> m_vector;
	public:
		IntStack() = default;
		IntStack(const IntStack&) = default;
		IntStack(IntStack&& other)noexcept :m_vector(std::move(other.m_vector)) {}

		void clear() {
			m_vector.clear();
		}

		void push(T t) {
			m_vector.push_back(t);
		}

		T& top() {
			return m_vector.back();
		}

		const T top()const {
			return m_vector.back();
		}

		void pop() {
			m_vector.pop_back();
		}

		size_t size()const {
			return m_vector.size();
		}

		bool empty()const {
			return m_vector.empty();
		}
	};


	template<class T>
	class SparseSet;

	template<class T>
		requires std::is_unsigned_v<T>
	class SparseSet<T> {
	private:
		using vector_t = IntVector<T>;

		vector_t dense;
		vector_t sparse;

		static constexpr T null_value = std::numeric_limits<T>::max();

	public:
		static constexpr size_t _max_size = 1'000'000;
		using const_iterator = vector_t::const_iterator;

		SparseSet() {}

		void insert(T num) {
			MYECS_ASSERT(num < _max_size, "number too big!");
			if (sparse.size() <= num) {
				sparse.resize(num + 1ull, null_value);
			}
			else if (sparse[static_cast<size_t>(num)] != null_value) {
				return;
			}
			dense.emplace_back(num);
			sparse[static_cast<size_t>(num)] = static_cast<T>(dense.size() - 1ull);
		}

		void erase(T num) {
			if (num >= sparse.size()) {
				return;
			}
			size_t index = sparse[num];
			if (index == null_value) {
				return;
			}

			T last_num = dense.back();
			dense[static_cast<size_t>(index)] = last_num;
			sparse[static_cast<size_t>(last_num)] = index;
			dense.pop_back();

			//in case when the dense vector is empty
			sparse[static_cast<size_t>(num)] = null_value;
		}

		void clear() {
			dense.clear();
			sparse.clear();
		}

		bool has(T num)const {
			if (static_cast<size_t>(num) >= sparse.size()) {
				return false;
			}
			return sparse[static_cast<size_t>(num)] != null_value;
		}

		size_t size()const {
			return dense.size();
		}

		size_t max_value_size()const {
			return sparse.size();
		}

		const_iterator begin() const { return dense.begin(); }
		const_iterator end() const { return dense.end(); }

		T operator[](size_t idx)const {
			return dense[idx];
		}
	};

	template<>
	class SparseSet<entity> {
	private:
		using dense_t = IntVector<entity>;
		using sparse_t = IntVector<size_t>;

		dense_t dense;
		sparse_t sparse;

		static constexpr size_t null_value = std::numeric_limits<size_t>::max();

	public:
		static constexpr size_t _max_size = 0x100000;
		using const_iterator = dense_t::const_iterator;

		SparseSet() {}

		void insert(entity e) {
			size_t id = static_cast<size_t>(e.id);
			MYECS_ASSERT(id < _max_size, "number too big!");
			if (sparse.size() <= id) {
				sparse.resize(id + 1ull, null_value);
			}
			else if (sparse[id] != null_value) {
				return;
			}
			dense.emplace_back(e);
			sparse[id] = dense.size() - 1ull;
		}

		void erase(entity e) {
			size_t id = static_cast<size_t>(e.id);
			if (id >= sparse.size()) {
				return;
			}
			size_t index = sparse[id];
			if (index == null_value) {
				return;
			}

			entity last = dense.back();
			dense[index] = last;
			sparse[static_cast<size_t>(last.id)] = index;
			dense.pop_back();

			//in case when the dense vector is empty
			sparse[id] = null_value;
		}

		void clear() {
			dense.clear();
			sparse.clear();
		}

		bool has(entity e)const {
			size_t id = static_cast<size_t>(e.id);
			if (id >= sparse.size()) {
				return false;
			}
			return sparse[id] != null_value && dense[sparse[id]] == e;
		}

		size_t size()const {
			return dense.size();
		}

		size_t max_value_size()const {
			return sparse.size();
		}

		const_iterator begin() const { return dense.begin(); }
		const_iterator end() const { return dense.end(); }
	};

	template<class T>
	class IdGen;

	template<class T>
		requires std::is_unsigned_v<T>
	class IdGen<T> {
	private:
		using u32 = types::u32;
		static constexpr u32 invalid_id = -1;

		struct Node {
			bool valid = false;
		};

		IntStack<size_t> unused_id;
		clever_vector<Node> sparse;
		size_t m_count = 0;
	public:
		IdGen() {}
		IdGen(IdGen&& other)noexcept :
			unused_id(std::move(other.unused_id)),
			sparse(std::move(other.sparse)),
			m_count(other.m_count) {
			other.m_count = 0;
		}

		T get() {
			if (unused_id.empty()) {
				sparse.emplace_back(true);
				return m_count++;
			}
			size_t new_id = unused_id.top();
			unused_id.pop();
			m_count++;
			sparse[new_id].valid = true;
			return new_id;
		}

		void ret(T id) {
			if (active(id)) {
				sparse[id].valid = false;
				unused_id.push(id);
				m_count--;
			}
		}

		bool active(T id)const {
			return id < sparse.size()
				&& sparse[id].valid;
		}

		size_t count()const {
			return m_count;
		}

		size_t max_count()const {
			return sparse.size();
		}

		void clear() {
			unused_id.clear();
			sparse.clear();
			m_count = 0;
		}

		bool full()const {
			return unused_id.empty();
		}
	};

	template<>
	class IdGen<entity> {
	private:
		using u32 = types::u32;
		static constexpr u32 invalid_id = -1;
		struct Node {
			bool valid = false;
			u32 version = 0u;
		};

		IntStack<size_t> unused_id;
		clever_vector<Node> sparse;
		size_t m_count = 0;
	public:
		IdGen() {}
		IdGen(IdGen&& other)noexcept :
			unused_id(std::move(other.unused_id)),
			sparse(std::move(other.sparse)),
			m_count(other.m_count) {
			other.m_count = 0;
		}

		entity get() {
			if (unused_id.empty()) {
				sparse.emplace_back(true, 0u);
				return entity(static_cast<u32>(m_count++), 0u);
			}
			size_t new_id = unused_id.top();
			unused_id.pop();
			m_count++;
			sparse[new_id].valid = true;
			return entity(static_cast<u32>(new_id), sparse[new_id].version);
		}

		void ret(entity e) {
			if (active(e)) {
				sparse[e.id].version++;
				sparse[e.id].valid = false;
				unused_id.push(e.id);
				m_count--;
			}
		}

		bool active(entity e)const {
			return e.id < sparse.size()
				&& sparse[e.id].valid
				&& sparse[e.id].version == e.version;
		}

		size_t count()const {
			return m_count;
		}

		size_t max_count()const {
			return sparse.size();
		}

		void clear() {
			unused_id.clear();
			sparse.clear();
			m_count = 0;
		}

		bool full()const {
			return unused_id.empty();
		}
	};

}//namespace myecs

#endif