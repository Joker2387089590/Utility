#pragma once
#include <memory>

template<typename Sub>
class ShareOnly
{
protected:
	struct PrivateCtor { explicit PrivateCtor() = default; };
	inline static constexpr PrivateCtor ctor{};

	ShareOnly(ShareOnly&&) noexcept = default;
	ShareOnly& operator=(ShareOnly&&) noexcept = default;

public:
	explicit ShareOnly(PrivateCtor) noexcept {}
	ShareOnly(const ShareOnly&) = delete;
	ShareOnly& operator=(const ShareOnly&) = delete;

	static std::shared_ptr<Sub> Make()
	{
		return std::make_shared<Sub>(PrivateCtor{});
	}
};

template<typename Sub>
class UniqueOnly
{
protected:
	struct PrivateCtor { explicit PrivateCtor() = default; };
	UniqueOnly(UniqueOnly&&) noexcept = default;
	UniqueOnly& operator=(UniqueOnly&&) noexcept = default;

public:
	explicit UniqueOnly(PrivateCtor) noexcept {}
	UniqueOnly(const UniqueOnly&) = delete;
	UniqueOnly& operator=(const UniqueOnly&) = delete;

	static std::unique_ptr<Sub> Make()
	{
		return std::make_unique<Sub>(PrivateCtor{});
	}
};
