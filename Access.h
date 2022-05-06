#pragma once
#include <type_traits>
#include <utility>

namespace access
{
template<typename Dummy>
struct Tag { friend auto get_accessor(Tag); };

template<typename Tag>
struct TagTraits
{
	using tag_type = Tag;
	using accessor_type = decltype(get_accessor(std::declval<tag_type>()));
	using class_type = typename accessor_type::class_type;
	using access_type = typename accessor_type::access_type;
	using pointer_type = typename accessor_type::pointer_type;
};

template<typename T>
inline constexpr bool is_function_v =
	std::is_member_function_pointer_v<std::decay_t<T>> ||
	std::is_function_v<std::remove_pointer_t<T>>;

template<typename T>
struct get_pointing_type { using type = T; };

template<typename T>
struct get_pointing_type<T*> { using type = T; };

template<typename T, typename Class>
struct get_pointing_type<T Class::*> { using type = T; };

template<typename T>
using get_pointing_type_t = typename get_pointing_type<T>::type;

template<typename Tag, typename Class, typename Type, Type ptr_>
struct Accessor
{
	using class_type = Class;
	using access_type = Type;
	using pointer_type = decltype(ptr_);
	using const_pointer_type = std::add_const_t<pointer_type>;
	static constexpr pointer_type ptr = ptr_;

	friend auto get_accessor(Tag) { return Accessor{}; }
};

// call non-static member function
template<typename Tag, typename Target, typename... Args>
inline constexpr decltype(auto) call(Target&& target, Args&&... args)
{
	using tag_traits = TagTraits<Tag>;
	using access_type = typename tag_traits::access_type;
	using accessor_type = typename tag_traits::accessor_type;
	using pointer_type = typename tag_traits::pointer_type;

	static_assert(!std::is_function<std::remove_pointer_t<pointer_type>>::value,
				  "Static function doest not require a instance");
	static_assert(std::is_member_function_pointer<access_type>::value,
				  "Tag must represent non-static member function");
	return (std::forward<Target>(target)
			.*accessor_type::ptr)(std::forward<Args>(args)...);
}

// call static member function
template<typename Tag, typename... Args>
inline constexpr decltype(auto) call(Args&&... args)
{
	using tag_traits = TagTraits<Tag>;
	using access_type = typename tag_traits::access_type;
	using accessor_type = typename tag_traits::accessor_type;

	static_assert(
		!std::is_member_function_pointer<access_type>::value
			&& std::is_function<std::remove_pointer_t<access_type>>::value,
		"Tag must represent a static member function");
	return (accessor_type::ptr)(std::forward<Args>(args)...);
}

// get non-static member variable
template<typename Tag, typename Target>
inline constexpr auto get(Target& target) -> std::enable_if_t<
	!is_function_v<typename TagTraits<Tag>::access_type>,
	get_pointing_type_t<typename TagTraits<Tag>::access_type>&>
{
	return target.*TagTraits<Tag>::accessor_type::ptr;
}

template<typename Tag, typename Target>
inline constexpr auto get(const Target& target) -> std::enable_if_t<
	!is_function_v<typename TagTraits<Tag>::access_type>,
	get_pointing_type_t<typename TagTraits<Tag>::access_type> const&>
{
	return target.*TagTraits<Tag>::accessor_type::ptr;
}

template<typename Tag, typename Target>
inline constexpr auto get(Target&& target) -> std::enable_if_t<
	!is_function_v<typename TagTraits<Tag>::access_type>,
	get_pointing_type_t<typename TagTraits<Tag>::access_type>>
{
	return std::forward<Target>(target).*TagTraits<Tag>::accessor_type::ptr;
}

template<typename Tag, typename Target>
inline constexpr auto get(const Target&& target) -> std::enable_if_t<
	!is_function_v<typename TagTraits<Tag>::access_type>,
	get_pointing_type_t<typename TagTraits<Tag>::access_type>>
{
	return std::forward<Target>(target).*TagTraits<Tag>::accessor_type::ptr;
}

// get non-static member function pointer
template<typename Tag, typename Target>
inline constexpr auto get(Target&) -> std::enable_if_t<
	is_function_v<typename TagTraits<Tag>::access_type>,
	typename TagTraits<Tag>::access_type>
{
	return TagTraits<Tag>::accessor_type::ptr;
}

template<typename Tag, typename Target>
inline constexpr auto get(const Target&) -> std::enable_if_t<
	is_function_v<typename TagTraits<Tag>::access_type>,
	typename TagTraits<Tag>::access_type const>
{
	return TagTraits<Tag>::accessor_type::ptr;
}

template<typename Tag, typename Target>
inline constexpr auto get(Target&&) -> std::enable_if_t<
	is_function_v<typename TagTraits<Tag>::access_type>,
	typename TagTraits<Tag>::access_type>
{
	return TagTraits<Tag>::accessor_type::ptr;
}

template<typename Tag, typename Target>
inline constexpr auto get(const Target&&) -> std::enable_if_t<
	is_function_v<typename TagTraits<Tag>::access_type>,
	typename TagTraits<Tag>::access_type const>
{
	return TagTraits<Tag>::accessor_type::ptr;
}

// get static member (both variable and function)
template<typename Tag>
auto get() -> std::enable_if_t<
	!is_function_v<typename TagTraits<Tag>::access_type>,
	get_pointing_type_t<typename TagTraits<Tag>::access_type>&>
{
	return *TagTraits<Tag>::accessor_type::ptr;
}

template<typename Tag>
auto get() -> std::enable_if_t<
	is_function_v<typename TagTraits<Tag>::access_type>,
	typename TagTraits<Tag>::access_type>
{
	return *TagTraits<Tag>::accessor_type::ptr;
}

#define ACCESS_CONCAT_IMPL(x, y) x##_##y
#define ACCESS_CONCAT(x, y) ACCESS_CONCAT_IMPL(x, y)

#define ACCESS_ENABLE_TAG(tag, clazz, member) \
template struct ::access::Accessor< \
tag, clazz, decltype(&clazz::member), &clazz::member>

// class_name, member_name
#define ACCESS_CREATE_UNIQUE_TAG(clazz, member) \
ACCESS_ENABLE_TAG(::access::Tag<class ACCESS_CONCAT(clazz, member)>, clazz, member)

// tag_name, class_name, member_name
#define ACCESS_CREATE_TAG(tag, clazz, member) \
using tag = ::access::Tag<class ACCESS_CONCAT( \
	ACCESS_CONCAT( \
		ACCESS_CONCAT(access_dummy$1znq_baf, clazz), \
		member), \
	__LINE__)>; \
ACCESS_ENABLE_TAG(tag, clazz, member)

// class_name, member_name, template_parameters...(if necessary)
#define ACCESS_GET_UNIQUE_TAG(clazz, member) \
::access::Tag<ACCESS_CONCAT(clazz, member)>

} // namespace access
