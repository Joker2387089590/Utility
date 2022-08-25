#pragma once
#include <map>
#include <stdexcept>
#include <Utility/ObjectAddress.h>

struct ListNodeBase
{
public:
    ListNodeBase() noexcept : pre{this}, next{this} {}
    ListNodeBase(ListNodeBase* after) noexcept { insert(after); }

    void insert(ListNodeBase* after) noexcept
    {
        this->pre = std::exchange(after->pre, this);
        this->next = std::exchange(pre->next, this);
    }

    template<bool reset = false>
    void remove() noexcept
    {
        next->pre = this->pre;
        pre->next = this->next;
        if constexpr (reset) pre = next = this;
    }

    void move(ListNodeBase* after) noexcept
    {
        remove();
        insert(after);
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
    template<typename... As>
    ListNode(ListNodeBase* after, As&&... as) :
        Value(std::forward<As>(as)...),
        ListNodeBase{after}
    {}

    using ListNodeBase::insert;
    using ListNodeBase::move;
    using ListNodeBase::remove;
    using Value::value;
};

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

private:
    class MapIter
    {
        friend LinkMap;
        using Iter = typename Map::iterator;
    public:
        MapIter& operator++() { ++i; return *this; }
        MapIter& operator--() { --i; return *this; }

        Pair& operator*() const { return const_cast<Pair&>(*objAddr(&i->first, &Pair::first)); }
        Pair* operator->() const { return &*this; }

        operator bool() const { return i != m.end(); }
        bool operator==(const MapIter& r) const noexcept { return i == r.i; }

        MapIter begin()	const noexcept { return *this; }
        MapIter end()	const noexcept { return { m.end(), m }; }

    private:
        MapIter(Iter i, Map& m) : i(i), m(m) {}

    private:
        Iter i;
        Map& m;
    };

    class ListIter
    {
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
        ListIter& operator++() { check(); node = cast(node->next); return *this; }
        ListIter& operator--() { check(); node = node->pre; return *this; }

        Pair& operator*()		const { check(); return *objAddr(&node->value(), &Pair::second); }
        Proxy operator->()		const { check(); return Proxy{node}; }
        Pair& operator[](int i) const { return *std::next(*this, i); }

        operator bool()						const noexcept { return node != h; }
        bool operator==(const ListIter& r)	const noexcept { return node == r.node && h == r.h; }

        ListIter begin()	const noexcept { return *this; }
        ListIter end()		const noexcept { return { cast(h), h }; }

        const K& key() const { return (**this).first; }
        V& value() const { return (**this).second; }

    private:
        ListIter(Node* n, ListNodeBase* h) noexcept : node(n), h(h) {}
        void check() const
        {
            if(!*this) throw std::out_of_range("This iterator is out of range!");
        }
        static Node* cast(ListNodeBase* n) { return static_cast<Node*>(n); }

    private:
        Node* node;
        ListNodeBase* const h;
    };

public:
    LinkMap() : m(), h{} {};
    LinkMap(std::initializer_list<std::pair<K, V>> values) :
        LinkMap()
    {
        for(auto&& [key, value] : values)
            emplace(std::move(key), std::move(value));
    }

public:
    template<typename Key, typename... Vs> V& insert(Key&& k, Vs&&... vs);  // init
    template<typename Key, typename... Vs> V& emplace(Key&& k, Vs&&... vs); // ctor

    void erase(K& k);
    void erase(ListIter iter);
    void erase(MapIter iter);

    template<typename Key>
    bool contains(Key&& k) const noexcept { return m.find(k) != m.end(); }

    std::size_t size() const noexcept { return m.size(); }

    template<typename Key> V& at(Key&& k);

public:
    ListIter list() { return { static_cast<Node*>(h.next), &h }; }
    MapIter map() { return { m.begin(), m }; }

private:
    Map m;
    ListNodeBase h;
};

template<typename K, typename V>
template<typename Key, typename... Vs>
V& LinkMap<K, V>::insert(Key&& k, Vs&&... vs)
{
    auto [pos, b] = m.try_emplace(std::forward<Key>(k), &h,
                                  init, std::forward<Vs>(vs)...); []{}();
    return pos->second.value();
}

template<typename K, typename V>
template<typename Key, typename... Vs>
V& LinkMap<K, V>::emplace(Key&& k, Vs&&... vs)
{
    auto [pos, b] = m.try_emplace(std::forward<Key>(k), &h,
                                  ctor, std::forward<Vs>(vs)...); []{}();
    return pos->second.value();
}

template<typename K, typename V>
void LinkMap<K, V>::erase(K& k)
{
    auto pos = m.find(k);
    if(pos == m.end()) return;
    pos->second.remove();
    m.erase(pos);
}

template<typename K, typename V>
void LinkMap<K, V>::erase(ListIter iter)
{
    if(!iter) return;
    iter.node->remove();
    m.erase(iter.key());
}

template<typename K, typename V>
void LinkMap<K, V>::erase(MapIter iter)
{
    if(iter == m.end()) return;
    iter->second.remove();
    m.erase(iter);
}

template<typename K, typename V>
template<typename Key>
V& LinkMap<K, V>::at(Key&& k)
{
    auto pos = m.find(k);
    if(pos == m.end()) throw std::exception();
    return pos->second.value();
}
