#pragma once

#include <functional>

template <typename TReturnType, class... Args>
class Functor
{
public:

	Functor() = default;
	Functor(std::function<TReturnType(Args...)> callback)
		: mCallback(callback) {}

	template <class... CallArgs>
	void Call(CallArgs&&...args) const
	{
		mCallback(std::forward<CallArgs>(args)...);
	}

	void Set(std::function<TReturnType(Args...)> callback)
	{
		mCallback = callback;
	}

	void operator()(Args... args) const
	{
		Call(args...);
	}

private:

	std::function<TReturnType(Args...)> mCallback;

};
