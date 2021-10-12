#include <Interface.h>

template<typename Sub> struct Itest;

template<> 
struct Itest<void>
{
	virtual void foo(int) = 0;
	virtual ~Itest() = default;
};

template<typename Sub>
struct Itest
{
	struct Ptr;
	void foo(int) = delete;
};

template<typename Sub>
struct Itest<Sub>::Ptr : public PtrBase<Itest, Sub>
{
	using Base = PtrBase<Itest, Sub>;
	using Base::Base;
	void foo(int i) override { return this->template call<&Ptr::foo>(&Sub::foo, i); }
};

template<typename Sub>
struct I2 : Itest<Sub>
{
	struct Ptr : public Itest<Sub>::Ptr
	{
		using Base = typename Itest<Sub>::Ptr;
		using Base::Base;
	};
};

template<> struct I2<void> : public Itest<void> {};

struct S : IBase<S, I2>
{
	void foo(int) {}
};

int main()
{
	S s;
	s.foo(1);
	Ptr<Itest> p(&s);
    return 0;
}
