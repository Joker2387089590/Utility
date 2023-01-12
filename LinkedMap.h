#pragma once
#include <map>
#include <stdexcept>
#include <Utility/ObjectAddress.h>

struct ListNodeBase
{
public:
    ListNodeBase() noexcept : pre{this}, next{this} {}
    ListNodeBase(ListNodeBase* after) noexcept { insert(after); }
	~ListNodeBase() noexcept = default;

	ListNodeBase(const ListNodeBase&) = delete;
	ListNodeBase& operator=(const ListNodeBase&) & = delete;
	ListNodeBase(ListNodeBase&&) noexcept = delete;
	ListNodeBase& operator=(ListNodeBase&&) & = delete;

public:
    void insert(ListNodeBase* after) noexcept
    {
		this->pre = after->pre;
		this->next = after;
		pre->next = this;
		after->pre = this;
    }

    void remove() noexcept
    {
        next->pre = this->pre;
		pre->next = this->next;
    }

    void move(ListNodeBase* after) noexcept
    {
        remove();
        insert(after);
    }

	void reset()
	{
		pre = next = this;
	}

public:
    ListNodeBase* pre;
    ListNodeBase* next;
};

struct Init {} inline constexpr init;
struct Ctor {} inline constexpr ctor;

template<typename V, typename = void>
class StorageWrapper : private V
{
public:
    template<typename... As> StorageWrapper(Init, As&&... as) : V{std::forward<As>(as)...} {}
    template<typename... As> StorageWrapper(Ctor, As&&... as) : V(std::forward<As>(as)...) {}

    V& value() { return *this; }
    const V& value() const { return *this; }
};

template<typename V>
class StorageWrapper<V, std::enable_if_t<!std::is_class_v<V> || std::is_final_v<V>, void>>
{
public:
    template<typename... As> StorageWrapper(Init, As&&... as) : v{std::forward<As>(as)...} {}
    template<typename... As> StorageWrapper(Ctor, As&&... as) : v(std::forward<As>(as)...) {}

    V& value() { return v; }
    const V& value() const { return v; }

private:
    V v;
};

template<typename V>
class ListNode : private StorageWrapper<V>, public ListNodeBase
{
    using Value = StorageWrapper<V>;
public:
	template<typename Tag, typename... As>
	ListNode(ListNodeBase* after, Tag tag, As&&... as) :
		Value(tag, std::forward<As>(as)...),
        ListNodeBase{after}
    {}

    using ListNodeBase::insert;
    using ListNodeBase::move;
    using ListNodeBase::remove;
    using Value::value;
};

// 既可按插入顺序遍历，也可按 Key 值遍历的 Map
// 注意：如果不需要按 Key 值遍历，应该用 std::vector<std::pair<K, V>>!!!
template<typename K, typename V>
class LinkMap
{
private:
    using Node = ListNode<V>;
    using Map = std::map<K, Node, std::less<>>;

    using RealPair = std::pair<const K, Node>;
    using Pair = std::pair<const K, V>;
    static_assert(alignof (RealPair) == alignof (Pair));

    class ListIter;
    class MapIter;

public:
    LinkMap() : m(), h{} {};
    LinkMap(std::initializer_list<std::pair<K, V>> values) : LinkMap()
    {
        for(auto&& [key, value] : values) emplace(key, value);
    }

	LinkMap(const LinkMap&) = delete;
	LinkMap& operator=(const LinkMap&) & = delete;

public:
	using InsertResult = std::pair<V&, bool>;
	template<typename Key, typename... Vs> InsertResult insert(Key&& k, Vs&&... vs);  // init
	template<typename Key, typename... Vs> InsertResult emplace(Key&& k, Vs&&... vs); // ctor

	void erase(K& k) noexcept;
	void erase(ListIter iter) noexcept;
	void erase(MapIter iter) noexcept;
	void clear() noexcept { m.clear(); h.reset(); }

    template<typename Key>
    bool contains(Key&& k) const noexcept { return m.find(k) != m.end(); }

    std::size_t size() const noexcept { return m.size(); }

	template<typename Key> V& at(Key&& k);
	template<typename Key> const V& at(Key&& k) const;

	template<typename Key> V* tryGet(Key&& k) noexcept;
	template<typename Key> const V* tryGet(Key&& k) const noexcept;

    ListIter list() & { return { static_cast<Node*>(h.next), &h }; }
    MapIter map() & { return { m.begin(), m }; }

	template<typename Key> ListIter list(Key&& key) &;
	template<typename Key> std::size_t indexOf(Key&& key) const;

private:
    Map m;
    ListNodeBase h;
};

template<typename K, typename V>
class LinkMap<K, V>::MapIter : public Map::iterator
{
	friend LinkMap;
	using Iter = typename Map::iterator;
private:
	MapIter(Iter i, Map& m) : Iter(i), m(m) {}
	~MapIter() = default;

public:
	MapIter(const MapIter&) = default;
	MapIter& operator=(const MapIter&) & = default;

public:
	using Iter::operator++;
	using Iter::operator--;

	Pair& operator*() const
	{
		const Pair* pair = objAddr(&self()->first, &Pair::first);
		return const_cast<Pair&>(*pair);
	}

	Pair* operator->() const { return std::addressof(**this); }

	operator bool() const { return *this != m.end(); }

	bool operator==(const MapIter& r) const noexcept { return self() == r.self(); }

	MapIter begin()	const noexcept { return *this; }
	MapIter end()	const noexcept { return { m.end(), m }; }

private:
	using Iter::operator*;
	using Iter::operator->;

	Iter& self() { return *this; }
	const Iter& self() const { return *this; }

private:
	Map& m;
};

template<typename K, typename V>
class LinkMap<K, V>::ListIter
{
private:
	friend LinkMap;
	class Proxy
	{
	public:
		Pair* operator->() const { return objAddr(&node->value(), &Pair::second); }
	private:
		friend ListIter;
		Node* node;
	};

public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = Pair;
	using difference_type = std::ptrdiff_t;
	using pointer = Proxy;
	using reference = Pair&;

private:
	ListIter(Node* n, ListNodeBase* h) noexcept : node(n), h(h) {}

public:
	ListIter(ListIter&&) noexcept = default;
	ListIter& operator=(ListIter&&) & noexcept = default;
	ListIter(const ListIter&) noexcept = default;
	ListIter& operator=(const ListIter&) & noexcept = default;
	~ListIter() noexcept = default;

public: // pointer-like
	ListIter& operator++() { check(); node = cast(node->next); return *this; }
	ListIter& operator--() { check(); node = cast(node->pre);  return *this; }

	ListIter operator++(int) { check(); auto temp = node; node = cast(node->next); return {temp, h}; }
	ListIter operator--(int) { check(); auto temp = node; node = cast(node->pre);  return {temp, h}; }

	Pair& operator*()		const { check(); return *objAddr(&node->value(), &Pair::second); }
	Proxy operator->()		const { check(); return Proxy{node}; }
	Pair& operator[](int i) const { return *std::next(*this, i); }

	explicit operator bool() const noexcept { return node != h; }

	bool operator==(const ListIter& r) const noexcept { return (node == r.node) && (h == r.h); }
	bool operator!=(const ListIter& r) const noexcept { return !(*this == r); }

public: // range-like
	ListIter begin() const noexcept { return *this; }
	ListIter end()   const noexcept { return { cast(h), h }; }

public:
	const K& key() const { return (**this).first; }
	V& value()     const { return (**this).second; }

	std::size_t index() const
	{
		check();
		auto begin = ListIter(cast(h->next), h);
		return std::distance(begin, *this);
	}

private:
	void check() const
	{
		if(node == h) throw std::out_of_range("This iterator is out of range!");
	}

	static Node* cast(ListNodeBase* n) noexcept { return static_cast<Node*>(n); }

private:
	Node* node;
	ListNodeBase* const h;
};

template<typename K, typename V>
template<typename Key, typename... Vs>
std::pair<V&, bool> LinkMap<K, V>::insert(Key&& k, Vs&&... vs)
{
	auto [pos, b] =
		m.try_emplace(std::forward<Key>(k), &h, init, std::forward<Vs>(vs)...);
	return {pos->second.value(), b};
}

template<typename K, typename V>
template<typename Key, typename... Vs>
std::pair<V&, bool> LinkMap<K, V>::emplace(Key&& k, Vs&&... vs)
{
	auto [pos, b] =
		m.try_emplace(std::forward<Key>(k), &h, ctor, std::forward<Vs>(vs)...);
	return {pos->second.value(), b};
}

template<typename K, typename V>
void LinkMap<K, V>::erase(K& k) noexcept
{
    auto pos = m.find(k);
    if(pos == m.end()) return;
    pos->second.remove();
    m.erase(pos);
}

template<typename K, typename V>
void LinkMap<K, V>::erase(ListIter iter) noexcept
{
    if(!iter) return;
    iter.node->remove();
    m.erase(iter.key());
}

template<typename K, typename V>
void LinkMap<K, V>::erase(MapIter iter) noexcept
{
    if(iter == m.end()) return;
    iter->second.remove();
    m.erase(iter);
}

template<typename K, typename V>
template<typename Key>
const V& LinkMap<K, V>::at(Key&& k) const
{
    auto pos = m.find(k);
	if(pos == m.cend()) throw std::exception();
    return pos->second.value();
}

template<typename K, typename V>
template<typename Key>
V& LinkMap<K, V>::at(Key&& k)
{
	auto pos = m.find(k);
	if(pos == m.cend()) throw std::exception();
	return pos->second.value();
}

template<typename K, typename V>
template<typename Key> V* LinkMap<K, V>::tryGet(Key&& k) noexcept
{
	auto pos = m.find(k);
	if(pos == m.cend())
		return nullptr;
	else
		return &pos->second.value();
}

template<typename K, typename V>
template<typename Key> const V* LinkMap<K, V>::tryGet(Key&& k) const noexcept
{
	auto pos = m.find(k);
	if(pos == m.cend())
		return nullptr;
	else
		return &pos->second.value();
}

template<typename K, typename V>
template<typename Key>
auto LinkMap<K, V>::list(Key&& key) & -> LinkMap<K, V>::ListIter
{
	auto pos = m.find(key);
	if(pos == m.cend()) throw std::exception();
	return {&pos->second, &h};
}

template<typename K, typename V>
template<typename Key>
std::size_t LinkMap<K, V>::indexOf(Key&& key) const
{
	auto& self = const_cast<LinkMap&>(*this);
	auto pos = self.m.find(key);
	if(pos == m.cend()) throw std::exception();
	return ListIter{&pos->second, &self.h}.index();
}
