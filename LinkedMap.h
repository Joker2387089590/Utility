#pragma once
#include <map>
#include <list>
#include <deque>
#include <algorithm>
#include <stdexcept>
#include "ObjectAddress.h"

template<typename K, typename V>
class LinkedMap
{
	// list 防止迭代器失效
	using ValueList = std::list<std::pair<K, V>>;
public:
	LinkedMap() : m_values(), m_maps() {}
	LinkedMap(LinkedMap&&) noexcept = default;
	LinkedMap& operator=(LinkedMap&&) noexcept = default;

	LinkedMap(const LinkedMap& other) : m_values(other.m_values)
	{
		// 重建 map 迭代器
		for(auto i = m_values.begin(); i != m_values.end(); ++i)
			m_maps.insert({ i->first, i });
	}
	LinkedMap& operator=(const LinkedMap& other)
	{
		if(&other != this)
		{
			m_values = other.m_values;
			for(auto i = m_values.begin(); i != m_values.end(); ++i)
				m_maps.insert({ i->first, i });
		}
	}

	template<typename... Vs>
	auto emplaceFront(K key, Vs&&... vs)
	{
		if(auto pos = m_maps.find(key); pos == m_maps.end())
		{
			m_values.emplace_front(key, V(std::forward<Vs>(vs)...));
			auto newPos = m_values.begin();
			m_maps.insert({ std::move(key), newPos });
			return newPos;
		}
		else
		{
			pos->second->second = V(std::forward<Vs>(vs)...);
			return pos->second;
		}
	}
	template<typename... Vs>
	auto emplaceBack(K key, Vs&&... vs)
	{
		if(auto pos = m_maps.find(key); pos == m_maps.end())
		{
			m_values.emplace_back(key, V(std::forward<Vs>(vs)...));
			auto newPos = m_values.rbegin().base();
			m_maps.insert({ std::move(key), newPos });
			return newPos;
		}
		else
		{
			pos->second->second = V(std::forward<Vs>(vs)...);
			return pos->second;
		}
	}

	void pushFront(K key, V value)
	{
		emplaceFront(std::move(key), std::move(value));
	}
	void pushBack(K key, V value)
	{
		emplaceBack(std::move(key), std::move(value));
	}

	void popFront()
	{
		if(size() == 0) throw std::out_of_range("pop empty linked map!");
		const K& key = m_values.begin()->first;
		m_maps.erase(key);
		m_values.pop_front();
	}
	void popBack()
	{
		if(size() == 0) throw std::out_of_range("pop empty linked map!");
		const K& key = m_values.rbegin()->first;
		m_maps.erase(key);
		m_values.pop_back();
	}

	std::pair<K, V> takeFirst()
	{
		if(size() == 0) throw std::out_of_range("take empty linked map!");
		std::pair<K, V> front = std::move(*begin());
		m_maps.erase(front.first);
		m_values.pop_front();
		return front;
	}
	std::pair<K, V> takeLast()
	{
		if(size() == 0) throw std::out_of_range("take empty linked map!");
		std::pair<K, V> last = std::move(*rbegin());
		m_maps.erase(last.first);
		m_values.pop_back();
		return last;
	}
	std::pair<K, V> take(const K& key)
	{
		auto pos = m_maps.find(key);
		if (pos == m_maps.end()) throw std::out_of_range("can't find key!");
		std::pair<K, V> element = std::move(*(pos->second));
		m_values.erase(pos->second);
		m_maps.erase(pos);
		return element;
	}

	// 按 list 顺序，像 map 一样遍历
	auto begin()	noexcept		{ return m_values.begin();  }
	auto end()		noexcept		{ return m_values.end();    }
	auto rbegin()	noexcept		{ return m_values.rbegin(); }
	auto rend()		noexcept		{ return m_values.rend();   }

	auto begin()	const noexcept	{ return m_values.begin();  }
	auto end()		const noexcept	{ return m_values.end();    }
	auto rbegin()	const noexcept	{ return m_values.rbegin(); }
	auto rend()		const noexcept	{ return m_values.rend();   }

	// map 访问
	V& operator[](const K& key) { return at(key); }
	V& at(const K& key)
	{
		return m_maps.at(key)->second;
	}
	const V& operator[](const K& key) const { return at(key); }
	const V& at(const K& key) const { return m_maps.at(key)->second; }

	// vector 访问
	V& atIndex(std::size_t i)
	{
		if(i < size())
			return std::next(begin(), i)->second;
		else
			throw std::out_of_range("index is larger than size!");
	}
	const V& atIndex(std::size_t i) const
	{
		if(i < size())
			return std::next(begin(), i)->second;
		else
			throw std::out_of_range("index is larger than size!");
	}

	// 交换元素
	template<typename Iter>
	void swapElement(Iter l, Iter r)
	{
		auto rightNext = std::next(r);
		m_values.splice(l, *this, r);
		m_values.splice(rightNext, *this, l);

		auto mapLeft = m_maps.find(l->first);
		auto mapRight = m_maps.find(r->first);
		std::swap(mapLeft->second, mapRight->second);
	}

	auto find(const K& key)
	{
		auto pos = m_maps.find(key);
		if (pos != m_maps.end())
			return pos->second;
		else
			return m_values.end();
	}
	auto find(const K& key) const
	{
		auto pos = m_maps.find(key);
		if (pos != m_maps.end())
			return ValueList::const_iterator(pos->second);
		else
			return m_values.end();
	}

	// 容器通用
	std::size_t size() const noexcept { return m_values.size(); }
	void clear() noexcept
	{
		m_maps.clear();
		m_values.clear();
	}

private:
	ValueList m_values;
	std::map<K, typename ValueList::iterator> m_maps;
};

struct NodeBase
{
public:
	NodeBase() : pre(this), next(this) {}

	NodeBase(const NodeBase&) = delete;
	NodeBase& operator=(const NodeBase&) = delete;

	inline void insert(NodeBase& nextNode)
	{
		NodeBase& preNode = *nextNode.pre;
		preNode.next	= this;
		nextNode.pre	= this;
		this->pre		= &preNode;
		this->next		= &nextNode;
	}

	inline void erase()
	{
		NodeBase* p = pre;
		NodeBase* n = next;
		pre->next = n;
		next->pre = p;
		// this->pre = this->next = this;
	}

public:
	NodeBase* pre;
	NodeBase* next;
};

template<typename Sub>
class ListIterBase
{
public:
	explicit ListIterBase(NodeBase* node) : p(node) {}
	ListIterBase(const ListIterBase&) noexcept = default;
	ListIterBase(ListIterBase&& other) noexcept { std::swap(p, other.p); }

	Sub operator++()	{ p = p->next; return *this; }
	Sub operator--()	{ p = p->pre;  return *this; }
	Sub operator++(int)	{ Sub r = self(); p = p->next; return r; }
	Sub operator--(int)	{ Sub r = self(); p = p->pre;  return r; }

private:
	Sub& self() { return static_cast<Sub&>(*this); }

protected:
	NodeBase* p;
};

template<typename K, typename V>
class LinkedMultiMap
{
private:
	/// 节点类型
	class Node : public NodeBase
	{
	public:
		template<typename... As>
		Node(As&&... as) : NodeBase(), value(std::forward<As>(as)...) {}

	public:
		V value;
	};

	/// 内部容器类型
	using Container = std::map<K, Node>;

	/// 内部容器的迭代器
	using RawIter = typename Container::iterator;

	/// 迭代器解引用的返回类型
	using RefPair = std::pair<const K&, V&>;

	/// 迭代器仿指针用法
	class PtrPair
	{
	public:
		RefPair& operator*() { return refs; }
		RefPair* operator->() { return &refs; }

	private:
		PtrPair(const K& k, V& v) : refs{k, v} {}
		RefPair refs;
	};

public:
	/// List 迭代器
	class ListIter;

	/// Map 迭代器
	class MapIter
	{
	public:
		RefPair operator*() const { return {iter->first, iter->second.value}; }
		PtrPair operator->() const { return {iter->first, iter->second.value}; }

		MapIter& operator++()    { ++iter; return *this; }
		MapIter& operator--()    { --iter; return *this; }
		MapIter  operator++(int) { return iter++; }
		MapIter  operator--(int) { return iter--; }

		ListIter toListIter() const { return ListIter(c, &iter->second); }

	private:
		friend class ListIter;
		MapIter(LinkedMultiMap* m, RawIter i) : c(m), iter(std::move(i)) {}

	private:
		LinkedMultiMap* c;
		RawIter iter;
	};

	class ListIter : public ListIterBase<ListIter>
	{
		using Base = ListIterBase<ListIter>;
		friend LinkedMultiMap;

	public:
		ListIter(const ListIter&) noexcept = default;
		ListIter(ListIter&&) noexcept = default;

		RefPair operator*() { return c->getPairAddr(this->p); }
		PtrPair operator->() { return c->getPairAddr(this->p); }

		using Base::operator++;
		using Base::operator--;

		MapIter toMapIter() const
		{
			auto& [key, value] = **this;
			return MapIter(m_values.find(key), c);
		}

	private:
		friend class MapIter;
		ListIter(LinkedMultiMap* m, NodeBase* node) : Base(node), c(m) {}

	private:
		using Base::p;
		LinkedMultiMap* c;
	};

private:
	RefPair getPairAddr(NodeBase* node)
	{
		if(node == &m_node) throw std::exception();
		using MapNodeType = typename Container::value_type;
		auto addr = getObjAddr(static_cast<Node*>(node), &MapNodeType::second);
		return RefPair(addr->first, addr->second.value);
	}

public:
	LinkedMultiMap() : m_node() {}

public:
	ListIter listBegin() { return ListIter(this, m_node.next); }
	ListIter listEnd() { return ListIter(this, &m_node); }
	MapIter mapBegin() { return MapIter(m_values.begin(), this); }
	MapIter mapEnd() { return MapIter(m_values.end(), this); }

private:
	template<typename Key, typename... As>
	MapIter emplaceMap(Key&& key, As&&... as)
	{
		auto [pos, b] = m_values.try_emplace(std::forward<Key>(key),
											 std::forward<As>(as)...);
		if(!b) throw std::exception();
		return pos;
	}

public:
	template<typename Key, typename... As>
	void pushBack(Key&& key, As&&... as)
	{
		auto pos = emplaceMap(std::forward<Key>(key), std::forward<As>(as)...);
		pos->second.insert(m_node);
	}

	template<typename Key, typename... As>
	void pushFront(Key&& key, As&&... as)
	{
		auto pos = emplaceMap(std::forward<Key>(key), std::forward<As>(as)...);
		pos->second.insert(*m_node.next);
	}

	void popFront()
	{
		if(size() == 0) throw std::exception();
		auto* frontNode = m_node.next;
		frontNode->erase();
		m_values.erase(ListIter(this, frontNode).toMapIter().iter);
	}

	void popBack()
	{
		if(size() == 0) throw std::exception();
		auto* backNode = m_node.pre;
		backNode->erase();
		m_values.erase(ListIter(this, backNode).toMapIter().iter);
	}

	bool erase(const K& key)
	{
		auto pos = m_values.find(key);
		if(pos == m_values.end()) return false;
		pos->second.erase();
		m_values.erase(pos);
		return true;
	}

	MapIter erase(MapIter i)
	{
		i->second.erase();
		return m_values.erase(i);
	}

	ListIter erase(ListIter i)
	{
		ListIter cur = i++;
		cur.p->erase();
		m_values.erase(cur->key);
		return i;
	}

	auto size() const noexcept { return m_values.size(); }

private:
	NodeBase m_node;
	Container m_values;
};
