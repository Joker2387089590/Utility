#define fwd(arg) std::forward<decltype(arg)>(arg)

#define DefaultClass(T)                     \
	T(T&&) noexcept = default;              \
	T& operator=(T&&) & noexcept = default; \
	T(const T&) = default;                  \
	T& operator=(const T&) & = default;     \
	~T() noexcept = default;                \

#define DefaultMove(T)                      \
	T(T&&) noexcept = default;              \
	T& operator=(T&&) & noexcept = default; \
	T(const T&) = delete;                   \
	T& operator=(const T&) & = delete;      \
	~T() noexcept = default;                \

#define DeleteCtor(T)                      \
	T(T&&) noexcept = delete;              \
	T& operator=(T&&) & noexcept = delete; \
	T(const T&) = delete;                  \
	T& operator=(const T&) & = delete;     \
