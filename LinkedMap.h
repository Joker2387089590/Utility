#pragma once
#include <map>
#include <cmath>
#include <numbers>
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

using Real = double;
struct Imag;
struct Complex;

struct Imag
{
    constexpr Imag(double v) noexcept : value(v) {}
    constexpr Imag& operator=(double v) noexcept { value = v; return *this; }

    constexpr Imag(const Imag&) noexcept = default;
    constexpr Imag(Imag&&) noexcept = default;
    constexpr Imag& operator=(const Imag&) noexcept = default;
    constexpr Imag& operator=(Imag&&) noexcept = default;
    constexpr ~Imag() noexcept = default;

    constexpr operator Complex() const noexcept;
    constexpr const double& operator()() const noexcept { return value; }

    constexpr Imag operator-() const noexcept { return -value; }
    constexpr Imag operator~() const noexcept { return -value; }

    double value;
};

struct Complex
{
    constexpr Complex(Real r, Imag i = 0.) noexcept : real{r}, imag(i) {}
    constexpr Complex(Imag i) noexcept : real(0.), imag(i) {}

    constexpr Complex(const Complex&) noexcept = default;
    constexpr Complex(Complex&&) noexcept = default;
    constexpr Complex& operator=(const Complex&) noexcept = default;
    constexpr Complex& operator=(Complex&&) noexcept = default;
    constexpr ~Complex() noexcept = default;

    constexpr Complex operator-() const noexcept { return {-real, -imag}; }
    constexpr Complex operator~() const noexcept { return {real, -imag}; }
    constexpr double abs() const noexcept { return std::sqrt(real * real + imag() * imag()); }

    Real real;
    Imag imag;
};

struct PolarComplex
{
    constexpr PolarComplex(Real r, Real theta) : r(r), theta(theta) {}
    constexpr PolarComplex(Complex c) : r(c.abs()), theta(std::atan2(c.imag(), c.real)) {}

    constexpr PolarComplex(const PolarComplex&) noexcept = default;
    constexpr PolarComplex(PolarComplex&&) noexcept = default;
    constexpr PolarComplex& operator=(const PolarComplex&) noexcept = default;
    constexpr PolarComplex& operator=(PolarComplex&&) noexcept = default;
    constexpr ~PolarComplex() noexcept = default;

    constexpr operator Complex() const noexcept { return {r * std::cos(theta), r * std::sin(theta)}; }

    constexpr PolarComplex operator-() const noexcept { return {r, theta + std::numbers::pi}; }
    constexpr PolarComplex operator~() const noexcept { return {r, -theta}; }
    constexpr double abs() const noexcept { return std::abs(r); }

    Real r;
    Real theta;
};

inline constexpr Imag::operator Complex() const noexcept { return {0, value}; }

inline constexpr Imag operator""_i(long double imag) noexcept { return double(imag); }
inline constexpr Imag operator""_i(unsigned long long int imag) noexcept { return double(imag); }

inline constexpr Imag    operator+(Imag a, Imag b)       noexcept { return {a.value + b.value}; }
inline constexpr Complex operator+(Real r, Imag i)       noexcept { return {r, i}; }
inline constexpr Complex operator+(Imag i, Real r)       noexcept { return {r, i}; }
inline constexpr Complex operator+(Complex c, Imag i)    noexcept { return {c.real, c.imag + i}; }
inline constexpr Complex operator+(Imag i, Complex c)    noexcept { return {c.real, c.imag + i}; }
inline constexpr Complex operator+(Real r, Complex c)    noexcept { return {c.real + r, c.imag}; }
inline constexpr Complex operator+(Complex c, Real r)    noexcept { return {c.real + r, c.imag}; }
inline constexpr Complex operator+(Complex a, Complex b) noexcept { return {a.real + b.real, a.imag + b.imag}; }

inline constexpr Imag    operator-(Imag a, Imag b)       noexcept { return {a.value - b.value}; }
inline constexpr Complex operator-(Real r, Imag i)       noexcept { return {r, -i}; }
inline constexpr Complex operator-(Imag i, Real r)       noexcept { return {-r, i}; }
inline constexpr Complex operator-(Complex c, Imag i)    noexcept { return {c.real, c.imag - i}; }
inline constexpr Complex operator-(Imag i, Complex c)    noexcept { return -(c - i); }
inline constexpr Complex operator-(Real r, Complex c)    noexcept { return r + -c; }
inline constexpr Complex operator-(Complex c, Real r)    noexcept { return {c.real - r, c.imag }; }
inline constexpr Complex operator-(Complex a, Complex b) noexcept { return {a.real - b.real, a.imag - b.imag}; }

inline constexpr Real    operator*(Imag a, Imag b)       noexcept { return -a.value * b.value; }
inline constexpr Imag    operator*(Imag i, Real r)       noexcept { return i.value * r; }
inline constexpr Imag    operator*(Real r, Imag i)       noexcept { return i.value * r; }
inline constexpr Complex operator*(Complex c, Imag i)    noexcept { return c.real * i + c.imag * i; }
inline constexpr Complex operator*(Imag i, Complex c)    noexcept { return c.real * i + c.imag * i; }
inline constexpr Complex operator*(Complex c, Real r)    noexcept { return c.real * r + c.imag * r; }
inline constexpr Complex operator*(Real r, Complex c)    noexcept { return c.real * r + c.imag * r; }
inline constexpr Complex operator*(Complex a, Complex b) noexcept { return {a.real * b.real + a.imag * b.imag, a.real * b.imag + a.imag * b.real}; }

inline constexpr Real    operator/(Imag a, Imag b)       noexcept { return a.value / b.value; }
inline constexpr Imag    operator/(Imag i, Real r)       noexcept { return i.value / r; }
inline constexpr Imag    operator/(Real r, Imag i)       noexcept { return r / i.value; }
inline constexpr Complex operator/(Complex c, Real r)    noexcept { return { c.real / r, c.imag / r }; }
inline constexpr Complex operator/(Real r, Complex c)    noexcept { return (r * ~c) / (c.real * c.real + c.imag() * c.imag()); }
inline constexpr Complex operator/(Complex c, Imag i)    noexcept { return c * ~i / (i.value * i.value); }
inline constexpr Complex operator/(Imag i, Complex c)    noexcept { return i * ~c / (c.real * c.real + c.imag() * c.imag()); }
inline constexpr Complex operator/(Complex a, Complex b) noexcept { return a * ~b / (b.real * b.real + b.imag() * b.imag()); }

inline constexpr Imag&    operator+=(Imag& a, Imag b)       noexcept { return a = a + b; }
inline constexpr Complex& operator+=(Complex& c, Imag i)    noexcept { return c = c + i; }
inline constexpr Complex& operator+=(Complex& c, Real r)    noexcept { return c = c + r; }
inline constexpr Complex& operator+=(Complex& c, Complex o) noexcept { return c = c + o; }

inline constexpr Imag&    operator-=(Imag& a, Imag b)       noexcept { return a = a - b; }
inline constexpr Complex& operator-=(Complex& c, Imag i)    noexcept { return c = c - i; }
inline constexpr Complex& operator-=(Complex& c, Real r)    noexcept { return c = c - r; }
inline constexpr Complex& operator-=(Complex& c, Complex o) noexcept { return c = c - o; }

inline constexpr Imag&    operator*=(Imag& a, Real b)       noexcept { return a = a * b; }
inline constexpr Complex& operator*=(Complex& c, Imag i)    noexcept { return c = c * i; }
inline constexpr Complex& operator*=(Complex& c, Real r)    noexcept { return c = c * r; }
inline constexpr Complex& operator*=(Complex& c, Complex o) noexcept { return c = c * o; }

inline constexpr Imag&    operator/=(Imag& a, Real b)       noexcept { return a = a / b; }
inline constexpr Complex& operator/=(Complex& c, Imag i)    noexcept { return c = c / i; }
inline constexpr Complex& operator/=(Complex& c, Real r)    noexcept { return c = c / r; }
inline constexpr Complex& operator/=(Complex& c, Complex o) noexcept { return c = c / o; }

template<typename I, std::enable_if_t<std::is_integral_v<I> && !std::is_same_v<I,bool>, int> = 0>
inline constexpr Complex operator,(I r, Imag i) noexcept { return {double(r), i}; }
