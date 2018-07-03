#pragma once

#include <Perseus/Threading.h>

#include <Utils/Functor.h>

namespace Perseus
{
	class Job
	{
	public:
		using Task = Functor<void>;

	public:
		Job();
		explicit Job(Task task);
		Job(const Job& other);
		Job(Job&& other);
		~Job();

		void Run();

		Task GetTask() const;

		void operator=(const Job& other)
		{
			mTask = other.mTask;
		}

	private:
		Task mTask;
		Task mOnFinished;
	};
}