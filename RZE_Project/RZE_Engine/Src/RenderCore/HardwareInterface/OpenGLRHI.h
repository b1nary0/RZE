#pragma once

class OpenGLRHI
{

public:

	static OpenGLRHI& Get()
	{
		static OpenGLRHI instance;
		return instance;
	}

	void ClearColor(float r, float g, float b, float a);
	void Clear(const GLbitfield bitfield);

private:

	OpenGLRHI();
	~OpenGLRHI();

};
