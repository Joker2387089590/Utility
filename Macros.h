#define fwd(arg) std::forward<decltype(arg)>(arg)

#define DefaultClass(T)                     \
	T(T&&) noexcept = default;              \
	T& operator=(T&&) & noexcept = default; \
	T(const T&) = default;                  \
	T& operator=(const T&) & = default;     \
	~T() noexcept = default;                \
