#include <string>
#if defined(UTILITY_WINERROR_NO_QT) || !__has_include(<QString>)
#define UTILITY_WINERROR_HAS_QT 0
#else
#define UTILITY_WINERROR_HAS_QT 1
#include <QString>
#endif
#include <Windows.h>

namespace WinError
{
template<typename F>
concept Receiver = std::is_invocable_v<F, wchar_t*, std::size_t>;

template<Receiver F>
inline auto format(F&& f, ::DWORD error = ::GetLastError())
{
    using namespace std::string_view_literals;

	constexpr auto noError = L"";
	if(error == 0) return f(noError, 0);

    ::DWORD language;
    const int ret = ::GetLocaleInfoEx(
        LOCALE_NAME_SYSTEM_DEFAULT, LOCALE_ILANGUAGE | LOCALE_RETURN_NUMBER,
        reinterpret_cast<::LPWSTR>(&language),
        sizeof(language) / sizeof(wchar_t));
    if (ret == 0) language = 0;

    wchar_t* ptr = nullptr;
    const auto size = ::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr, error, language,
        reinterpret_cast<::LPWSTR>(&ptr), 0, nullptr);
    if(size == 0) 
    {
        constexpr auto err = L"unformatted error"sv;
        return f(err.data(), err.size());
    }

    struct Free
    {
        ~Free() { ::LocalFree(buffer); }
        wchar_t* buffer;
    };
    Free free{ptr};
    return f(ptr, std::size_t(size));
}

inline std::wstring wformat(::DWORD error = ::GetLastError())
{
    return format([](const wchar_t* msg, std::size_t size) { return std::wstring(msg, size); }, error);
}

#if UTILITY_WINERROR_HAS_QT
inline QString qformat(::DWORD error = ::GetLastError())
{
    return format(QString::fromWCharArray, error);
}
#endif

} // namespace WinError
