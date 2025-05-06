#ifdef UTILITY_MARCOS
#error MacrosUndef.h was not included since last inclusion of Macros.h!
#endif
#define UTILITY_MARCOS

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

#define UTILITY_X_PRAGMA(x) _Pragma(#x)

// warning
#if defined(__clang__)
#   define UTILITY_DISABLE_WARNING_PUSH UTILITY_X_PRAGMA(clang diagnostic push)
#   define UTILITY_DISABLE_WARNING_POP UTILITY_X_PRAGMA(clang diagnostic pop)
#   define UTILITY_DISABLE_WARNING(name) UTILITY_X_PRAGMA(clang diagnostic ignored name)
#   define UTILITY_DISABLE_WARNING_UNUSED_MACRO UTILITY_DISABLE_WARNING("-Wunused-macros")
#   define UTILITY_DISABLE_WARNING_HEADER_HYGIENE UTILITY_DISABLE_WARNING("-Wheader-hygiene")
#elif defined(__GNUC__)
#   define UTILITY_DISABLE_WARNING_PUSH UTILITY_X_PRAGMA(GCC diagnostic push)
#   define UTILITY_DISABLE_WARNING_POP UTILITY_X_PRAGMA(GCC diagnostic pop)
#   define UTILITY_DISABLE_WARNING(name) UTILITY_X_PRAGMA(GCC diagnostic ignored name)
#   define UTILITY_DISABLE_WARNING_UNUSED_MACRO UTILITY_DISABLE_WARNING("-Wunused-macros")
#   define UTILITY_DISABLE_WARNING_HEADER_HYGIENE
#elif defined(_MSC_VER)
#   define UTILITY_DISABLE_WARNING_PUSH UTILITY_X_PRAGMA(warning(push))
#   define UTILITY_DISABLE_WARNING_POP UTILITY_X_PRAGMA(warning(pop))
#   define UTILITY_DISABLE_WARNING(warningNumber) UTILITY_X_PRAGMA(warning(disable: warningNumber))
#   define UTILITY_DISABLE_WARNING_UNUSED_MACRO
#   define UTILITY_DISABLE_WARNING_HEADER_HYGIENE
#else
#   define UTILITY_DISABLE_WARNING_PUSH
#   define UTILITY_DISABLE_WARNING_POP
#   define UTILITY_DISABLE_WARNING(...)
#   define UTILITY_DISABLE_WARNING_INVALID_OFFSETOF
#   define UTILITY_DISABLE_WARNING_UNUSED_MACRO
#   define UTILITY_DISABLE_WARNING_HEADER_HYGIENE
#endif
