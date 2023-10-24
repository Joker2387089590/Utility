#pragma once
#include <memory>
#include <stack>
#include <functional>

template<typename F>
class Cleanup
{
public:
    template<typename Fi>
    Cleanup(Fi&& f) : f(std::forward<Fi>(f)), doClean(true) {}

    ~Cleanup() { if(doClean) f(); }

	Cleanup(Cleanup&&) = default;
	Cleanup& operator=(Cleanup&&) & = default;

	// 复制会导致多次 cleanup
	Cleanup(const Cleanup&) = delete;
	Cleanup& operator=(const Cleanup&) & = delete;

public:
    F f;
    bool doClean;
};

template<typename F> Cleanup(F&&) -> Cleanup<std::decay_t<F>>;

// RollbackHandle 的辅助类
class RollBackHelper
{
public:
	virtual bool Work() = 0;
	virtual void RollBack() = 0;
};

// 封装回滚操作
class RollbackHandle
{
	// 回滚操作栈，回滚操作为 void() 类型的可调用对象或函数，
	// stack: Last-in First-out
	using FuncStack = std::stack<std::function<void()>>;

public:
	explicit RollbackHandle() : rollBacks(std::make_shared<FuncStack>()) {}
	RollbackHandle(const RollbackHandle&) = default;
	RollbackHandle& operator=(const RollbackHandle&) = default;
	RollbackHandle(RollbackHandle&&) noexcept = default;
	RollbackHandle& operator=(RollbackHandle&&) noexcept = default;
	~RollbackHandle() = default;

	template<typename Fw, typename Fr>
	void operator()(Fw&& Work, Fr&& RollBack)
	{
		// Work/RollBack 是 lambda 函数，所以后接()来运行
		if(!Work()) throw std::move(*this); // Work 失败，将自己抛出
		rollBacks->emplace(std::forward<Fr>(RollBack)); // Work 成功，RollBack 才会入栈
	}

	// 重载
	void operator()(RollBackHelper* helper)
	{
		(*this)([helper] { return helper->Work(); },
				[helper] { return helper->RollBack(); });
	}

	template<typename T>
	void operator()(T* t, bool (T::*Work)(), void (T::*Rollback)())
	{
		(*this)([=] { return t->*Work(); },
				[=] { return t->*Rollback(); });
	}

	// 执行保存在栈中的所有回滚操作
	void RollBackAll() const
	{
		while(!rollBacks->empty())
		{
			// top() 是最后压进来的的元素，因为元素为 lambda 函数，所以后接()来运行
			rollBacks->top()();
			// 提取元素
			rollBacks->pop();
		}
	}

	std::size_t size() const { return rollBacks->size(); }

private:
	// 捕获异常的时候，惯用 const T&，这里用 mutable 解除 const 限定
	mutable std::shared_ptr<FuncStack> rollBacks;
};

// Usage:
// void TestRollBack()
// try
// {
// 	RollbackHandle Try; // 创建一个回滚器
//
//	// 简短的 Lambda 风格
// 	Try([] { return true; }, [] { std::cout << "1. Lambda Rollback\n"; });
//
//  // 继承 RollBackHelper 的面对对象风格
//	struct Obj : public RollBackHelper
//	{
//		bool Work() override { return true; }
//		void RollBack() override { std::cout << "2. Object Rollback\n"; }
//	};
//  auto rollBackObj = new Obj;
// 	Try(rollBackObj);
//
//	// 这一步抛异常，前两个回滚操作会执行，而这个不会
// 	Try([] { return false; }, [] { std::cout << "3. Not Rollback\n"; });
//
//	// 注意：在 Lambda 中使用引用捕获 try 语句内的对象，在 catch 子句中已经释放了！！
//	// 解决方法：先在 try 外面声明变量，再在 try 内引用捕获；或者用值捕获
//	int i = 0;
//  Try([] { return false; }, [&i] { std::cout << "Dead Object: " << i; });
// }
// catch(const RollbackHandle& Try)
// {
//	// 执行回滚
// 	Try.RollBackAll();
// }
