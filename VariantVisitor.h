#pragma once
#include <variant>

template<typename... Vs>
struct Visitor : Vs...
{
	template<typename... VsRef>
	Visitor(VsRef&&... visitors) : Vs(std::forward<VsRef>(visitors))... {}
	using Vs::operator()...;
};
template<typename... VsRef> Visitor(VsRef&&...) ->
	Visitor<std::remove_reference_t<std::remove_cv_t<VsRef>>...>;

template<typename T, typename... Vs>
decltype(auto) VisitBy(T&& variant, Vs&&... visitors)
{
	return std::visit(Visitor(std::forward<Vs>(visitors)...),
					  std::forward<T>(variant));
}
