#pragma once
#ifndef MYECS_DENSE_MAP
#define MYECS_DENSE_MAP

#include"container.h"
#include"utils.h"
#include<string>


namespace myecs {

	template<class Key, class Type, class Hash = std::hash<Key>, class KeyEq = std::equal_to<>, class Alloc = std::allocator<std::pair<const Key, Type>>>
	class DenseMap {
	private:
		using AllocTraits = std::allocator_traits<Alloc>;
		using Pair = std::pair<const Key, Type>;

		struct Node {
			Pair pair;
			size_t prev = invalid_index;
			size_t next = invalid_index;

			template<class _Pair>
			Node(_Pair&& pair, size_t prev = invalid_index, size_t next = invalid_index) :
				pair(std::forward<_Pair>(pair)),
				prev(prev),
				next(next) {
			}
			Node(const Node&) = default;
			Node(Node&& other)noexcept :
				pair(std::move(other.pair)),
				prev(other.prev),
				next(other.next) {
			}
			~Node() = default;
		};

		using Sparse = IntVector<size_t>;
		using Packed = std::vector<Node, typename AllocTraits::template rebind_alloc<Node>>;
		using node_iterator = typename Packed::iterator;
		using const_node_iterator = typename Packed::const_iterator;

		struct iterator :public Packed::iterator {
			using Super = Packed::iterator;

			iterator() :Super() {}
			iterator(const Packed::iterator& it) :Super(it) {}

			Pair& operator*() {
				return Super::operator*().pair;
			}

			Pair* operator->() {
				return &(Super::operator->()->pair);
			}
		};

		struct const_iterator :public Packed::const_iterator {
			using Super = Packed::const_iterator;

			const_iterator() :Super() {}
			const_iterator(const Packed::const_iterator& it) :Super(it) {}

			const Pair& operator*()const {
				return Super::operator*().pair;
			}

			const Pair* operator->()const {
				return &(Super::operator->()->pair);
			}
		};

		Sparse sparse;
		Packed packed;
		Hash myHash;
		KeyEq myKeyeq;
		bool should_rehash;

		static constexpr size_t min_bucket_size = 8;
		static constexpr float load_factor = 0.875;
		static constexpr size_t expand_factor = 2;
		static constexpr size_t invalid_index = std::numeric_limits<size_t>::max();

		template<class _Key>
		size_t get_bucket(const _Key& key) {
			return fast_mod(myHash(key), bucket_count());
		}

		void rehash_if_should() {
			if (should_rehash) {
				rehash();
			}
		}

		bool update_should_rehash() {
			return should_rehash = (load_factor < ((float)size() / (float)bucket_count()));
		}

		void rehash() {
			Packed oldData;
			oldData.swap(packed);
			size_t new_size = sparse.size() * expand_factor;
			sparse.clear();
			sparse.resize(new_size, invalid_index);
			for (auto& node : oldData) {
				move_no_check(node.pair);
			}
			should_rehash = false;
		}

		void move_no_check(Pair& pair) {
			emplace_no_check(get_bucket(pair.first), pair.first, std::move(pair.second));
		}

		template<class _Key = Key, class ...Args>
		node_iterator emplace_no_check(size_t bucket, _Key&& key, Args&&...args) {
			if (sparse[bucket] == invalid_index) {
				sparse[bucket] = packed.size();
				packed.emplace_back(Pair(std::forward<_Key>(key), Type(std::forward<Args>(args)...)));
			}
			else {
				size_t index = sparse[bucket];
				while (packed[index].next != invalid_index) {
					index = packed[index].next;
				}
				packed[index].next = packed.size();
				packed.emplace_back(Pair(std::forward<_Key>(key), Type(std::forward<Args>(args)...)),
									index,
									invalid_index);
			}
			return packed.begin() + packed.size() - 1;
		}

		template<class _Key = Key>
		node_iterator find(_Key&& key, size_t bucket) {
			size_t index = sparse[bucket];
			while (index != invalid_index) {
				if (myKeyeq(packed[index].pair.first, key)) {
					return packed.begin() + index;
				}
				index = packed[index].next;
			}
			return packed.end();
		}

	public:
		DenseMap() {
			sparse.resize(min_bucket_size, invalid_index);
			update_should_rehash();
		}
		DenseMap(const DenseMap&) = default;
		DenseMap(DenseMap&& other)noexcept :
			sparse(std::move(other.sparse)),
			packed(std::move(other.packed)),
			myHash(std::move(other.myHash)),
			myKeyeq(std::move(other.myKeyeq)),
			should_rehash(other.should_rehash) {
		}
		~DenseMap() {}

		size_t bucket_count()const {
			return sparse.size();
		}

		size_t size()const {
			return packed.size();
		}

		bool empty()const {
			return packed.empty();
		}

		iterator begin() {
			return packed.begin();
		}

		iterator end() {
			return packed.end();
		}

		const_iterator begin()const {
			return packed.begin();
		}

		const_iterator end()const {
			return packed.end();
		}

		void clear() {
			packed.clear();
			sparse.resize(min_bucket_size, invalid_index);
		}

		template<class _Key = Key>
		iterator find(const _Key& key) {
			size_t bucket = get_bucket(key);
			return find(key, bucket);
		}

		template<class _Key = Key, class ...Args>
		Pair& emplace_or_get(_Key&& key, Args&&...args) {
			rehash_if_should();
			size_t bucket = get_bucket(std::forward<_Key>(key));
			if (node_iterator it = find(key, bucket); it != packed.end()) {
				return it->pair;
			}
			auto& ret = emplace_no_check(bucket, std::forward<_Key>(key), std::forward<Args>(args)...)->pair;
			update_should_rehash();
			return ret;
		}

		template<class _Key = Key>
		Type& operator[](_Key&& key) {
			return emplace_or_get(std::forward<_Key>(key)).second;
		}

		template<class _Key = Key>
		void erase(_Key&& key) {
			size_t bucket = get_bucket(key);
			node_iterator it = find(key, bucket);
			if (it == packed.end()) {
				return;
			}
			size_t index = std::distance(packed.begin(), it);
			if (it->prev != invalid_index) {
				packed[it->prev].next = it->next;
			}
			else {
				sparse[bucket] = it->next;
			}
			if (it->next != invalid_index) {
				packed[it->next].prev = it->prev;
			}

			if (index == packed.size() - 1) {
				packed.pop_back();
				return;
			}
			it->~Node();
			Node& back = packed.back();
			if (back.prev != invalid_index) {
				packed[back.prev].next = index;
			}
			else {
				size_t back_bucket = get_bucket(back.pair.first);
				sparse[back_bucket] = index;
			}
			if (back.next != invalid_index) {
				packed[back.next].prev = index;
			}
			new (&packed[index]) Node(std::move(back));
			packed.pop_back();
		}
	};

}
#endif