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
	TReturnType Call(CallArgs&&...args) const
	{
		return mCallback(std::forward<CallArgs>(args)...);
	}

	void Set(std::function<TReturnType(Args...)> callback)
	{
		mCallback = callback;
	}

	TReturnType operator()(Args... args) const
	{
		return Call(args...);
	}

private:

	std::function<TReturnType(Args...)> mCallback;

};
