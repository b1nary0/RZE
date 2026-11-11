#pragma once

// SDL's video subsystem, which windows and displays need, set up as RZE has always behaved. Only Windowing uses it.
namespace SDLVideo
{
	// Starts SDL video if it isn't running, and keeps it running until a matching Release. False if it couldn't start.
	bool Acquire();
	void Release();

	// Keeps SDL video running for as long as it lives
	class Scope
	{
	public:
		Scope() : m_isAcquired(Acquire()) {}
		~Scope() { if (m_isAcquired) { Release(); } }

		Scope(const Scope&) = delete;
		Scope& operator=(const Scope&) = delete;

		bool IsAcquired() const { return m_isAcquired; }

	private:
		bool m_isAcquired;
	};
}
