#pragma once
#include <QEventLoop>

// 在调用线程创建一个事件循环，并等待一个信号发送再返回
// flag 可以决定，在 loop::exec 期间，本线程的不同种类信号，可否触发它连接的槽
// 注意可能发生死锁或竞争
template<typename T, typename... Args>
void WaitForSignal(T* sender,
				   void(T::*signal)(Args...),
				   QEventLoop::ProcessEventsFlag flag = QEventLoop::ExcludeUserInputEvents)
{
	QEventLoop loop;
	QObject::connect(sender, signal, &loop, &QEventLoop::quit);
	loop.exec(flag);
}
