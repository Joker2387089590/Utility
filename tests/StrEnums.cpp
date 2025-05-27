#include "../include/Utility/StrEnums.h"

int main()
{
    using namespace StrEnums;

    enum X { A, B, C };
    using T = Builder<X>
        ::Add<A, "A">
        ::Add<B, "B">
        ::Add<C, "C">
        ::Build;

    static_assert(T::enumOf<"A"> == A);
    static_assert(T::enumOf<"B"> == B);
    static_assert(T::enumOf<"C"> == C);
    static_assert(T::textOf<A> == "A");
    static_assert(T::textOf<B> == "B");
    static_assert(T::textOf<C> == "C");

    enum class Y { V = 10 };
    constexpr auto y = static_cast<Y>(10);
}
