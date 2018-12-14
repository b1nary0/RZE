#include <Diotima/Driver/OpenGL.h>

#include <Utils/DebugUtils/Debug.h>

void GLAPIENTRY
MessageCB(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	if (type == GL_DEBUG_OUTPUT)
	{
		LOG_CONSOLE(message);
	}
}

OpenGLRHI::OpenGLRHI()
{
}

OpenGLRHI::~OpenGLRHI()
{
}

void OpenGLRHI::Init(const OpenGLCreationParams& creationParams) const
{
	LOG_CONSOLE_ARGS("OpenGL version is %s", glGetString(GL_VERSION));

	// @implementation these need to be read in via window attributes
	glViewport(0, 0, creationParams.WindowWidth, creationParams.WindowHeight);

	InitGLEW();

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCB, 0);
}

void OpenGLRHI::InitGLEW() const
{
	glewExperimental = true;

	GLenum err = glewInit();
	if (err == GLEW_OK)
	{
		LOG_CONSOLE_ANNOUNCE("GLEW initialized successfully.");
	}
	else
	{
		// @todo do something meaningful here
		LOG_CONSOLE_ANNOUNCE("GLEW did not initialize.");
		AssertFalse();
	}
}

void OpenGLRHI::ClearColor(const GLfloat red, const GLfloat green, const GLfloat blue, const GLfloat alpha) const
{
	glClearColor(red, green, blue, alpha);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::Clear(const GLuint mask) const
{
	glClear(mask);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::EnableCapability(const GLenum capability) const
{
	glEnable(capability);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::DisableCapability(const GLenum capability) const
{
	glDisable(capability);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::Viewport(GLint posX, GLint posY, GLsizei sizeX, GLsizei sizeY) const
{
	glViewport(posX, posY, sizeX, sizeY);
}

void OpenGLRHI::SetSwapInterval(GLint val) const
{
	wglSwapIntervalEXT(val);
}

void OpenGLRHI::LogShaderInfo(const GLuint shaderProgramID) const
{
	GLint length;
	GetShaderiv(shaderProgramID, EGLShaderStatusParam::InfoLogLength, &length);

	GLchar* log = (GLchar*)malloc(length);
	GetShaderInfoLog(shaderProgramID, length, &length, log);

	if (length > 1)
	{
		LOG_CONSOLE_ARGS("%s", log);
	}
}

void OpenGLRHI::GenVertexArrays(const GLuint arrayCount, GLuint* outBufferHandle) const
{
	AssertExpr(arrayCount > 0);
	glGenVertexArrays(arrayCount, outBufferHandle);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::BindVertexArray(const GLuint arrayObjectHandle) const
{
	glBindVertexArray(arrayObjectHandle);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::GenerateBuffer(GLsizei bufferCount, GLuint* outBufferHandle) const
{
	AssertExpr(bufferCount > 0);
	glGenBuffers(bufferCount, outBufferHandle);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::GenerateFrameBuffer(GLsizei bufferCount, GLuint* outBufferHandle) const
{
	AssertExpr(bufferCount > 0);
	glGenFramebuffers(bufferCount, outBufferHandle);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::GenerateRenderBuffer(GLsizei bufferCount, GLuint* outBufferHandle) const
{
	AssertExpr(bufferCount > 0);
	glGenRenderbuffers(bufferCount, outBufferHandle);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::BindBuffer(const EGLBufferTarget::T target, const GLuint bufferObjectHandle) const
{
	glBindBuffer(target, bufferObjectHandle);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::BindFramebuffer(EGLBufferTarget::T bufferTarget, GLuint bufferObjectHandle) const
{
	glBindFramebuffer(bufferTarget, bufferObjectHandle);
	GLenum errorCode = glGetError();
	AssertExpr(errorCode == GL_NO_ERROR);
}

void OpenGLRHI::BindRenderbuffer(const GLint bufferObjectHandle) const
{
	glBindRenderbuffer(GL_RENDERBUFFER, bufferObjectHandle);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::DeleteBuffer(GLuint bufferCount, GLuint* bufferHandle) const
{
	AssertExpr(bufferCount > 0 && bufferHandle);
	glDeleteBuffers(bufferCount, bufferHandle);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::SetBufferData(const EGLBufferTarget::T target, const GLuint size, const void* const data, const EGLBufferUsage::T bufferUsage) const
{
	AssertExpr(size > 0);
	// @note if this doesnt work check the const void* const signature because its possible under the hood it no likey
	glBufferData(target, size, data, bufferUsage);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::SetBufferSubData(const EGLBufferTarget::T target, const GLintptr offset, const GLsizeiptr size, const GLvoid* data) const
{
	glBufferSubData(target, offset, size, data);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::SetFramebufferTexture2D(const EGLBufferTarget::T target, const EGLAttachmentPoint::T attachmentPoint, const EGLTextureTarget::T textureTarget, GLuint texture, GLint mipLevel) const
{
	AssertExpr(mipLevel >= 0);
	glFramebufferTexture2D(target, attachmentPoint, textureTarget, texture, mipLevel);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::AllocateRenderbufferStorage(GLenum internalFormat, GLsizei width, GLsizei height) const
{
	AssertExpr(width > 0 || height > 0); // At least one of these needs to be > 0 for anything to make sense and shit to work
	glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, width, height);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::AttachRenderBufferToFrameBuffer(const EGLAttachmentPoint::T attachmentPoint, GLuint bufferObjectHandle) const
{
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachmentPoint, GL_RENDERBUFFER, bufferObjectHandle);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::EnableVertexAttributeArray(const GLuint index) const
{
	glEnableVertexAttribArray(index);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::VertexAttribPointer(const GLuint index, const GLint size, const EGLDataType::T type, const GLboolean normalized, const GLuint stride, const void* const pointer) const
{
	AssertExpr(size > 0);
	glVertexAttribPointer(index, size, type, normalized, stride, pointer);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::DrawArrays(const EGLDrawMode::T mode, const GLint first, const GLuint count) const
{
	AssertExpr(count > 0);
	glDrawArrays(mode, first, count);
}

void OpenGLRHI::DrawElements(const EGLDrawMode::T mode, const GLsizei count, EGLDataType::T type, const GLvoid* indices) const
{
	AssertExpr(count > 0);
	glDrawElements(mode, count, type, indices);
}

void OpenGLRHI::CreateShaderProgram(GLuint& outProgramID) const
{
	outProgramID = glCreateProgram();
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::CreateShader(const EGLShaderType::T shaderType, GLuint& outShaderID) const
{
	outShaderID = glCreateShader(shaderType);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::DeleteShader(const GLuint shaderID) const
{
	glDeleteShader(shaderID);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::CompileShader(const GLuint shaderID) const
{
	glCompileShader(shaderID);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::AttachShader(const GLuint shaderProgramID, const GLuint shaderID) const
{
	glAttachShader(shaderProgramID, shaderID);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::UseShaderProgram(const GLuint shaderProgramID) const
{
	glUseProgram(shaderProgramID);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::SetShaderSource(const GLuint shaderID, const GLuint count, const GLchar* sourceCode, const GLint* length) const
{
	GLboolean bIsValidShader = false;
	IsShader(shaderID, bIsValidShader);

	AssertExpr(count > 0);
	AssertNotNull(sourceCode);

	glShaderSource(shaderID, count, &sourceCode, length);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::IsShader(const GLuint shaderID, GLboolean& outResult) const
{
	outResult = glIsShader(shaderID);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::IsShaderProgram(const GLuint shaderProgramID, GLboolean& outResult) const
{
	outResult = glIsProgram(shaderProgramID);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::LinkShaderProgram(const GLuint shaderProgramID) const
{
	glLinkProgram(shaderProgramID);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::GetProgramiv(const GLuint mShaderProgramID, const EGLShaderProgramStatusParam::T programStatusParam, GLint* programLinkStatus) const
{
	glGetProgramiv(mShaderProgramID, programStatusParam, programLinkStatus);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::GetShaderiv(const GLuint shaderID, const EGLShaderStatusParam::T shaderStatusParam, GLint* const params) const
{
	glGetShaderiv(shaderID, shaderStatusParam, params);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::UniformLocation(const GLuint shaderProgramID, const GLchar* uniformName, GLint& outUniformLocation) const
{
	outUniformLocation = glGetUniformLocation(shaderProgramID, uniformName);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::GetProgramInfoLog(const GLuint programID, const GLsizei maxLength, GLsizei* length, GLchar* infoLog) const
{
	glGetProgramInfoLog(programID, maxLength, length, infoLog);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::GetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog) const
{
	glGetShaderInfoLog(shader, maxLength, length, infoLog);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::SetUniformInt(const GLint uniformLocation, const int value) const
{
	if (uniformLocation >= 0)
	{
		glUniform1i(uniformLocation, value);
		AssertExpr(glGetError() == GL_NO_ERROR);
	}
}

void OpenGLRHI::SetUniformFloat(const GLint uniformLocation, const float value) const
{
	if (uniformLocation >= 0)
	{
		glUniform1f(uniformLocation, value);
		AssertExpr(glGetError() == GL_NO_ERROR);
	}
}

void OpenGLRHI::SetUniformMat4x4(const GLint uniformLocation, const GLsizei count, const GLboolean transpose, const GLfloat* valuePtr) const
{
	if (uniformLocation >= 0)
	{
		glUniformMatrix4fv(uniformLocation, count, transpose, valuePtr);
		AssertExpr(glGetError() == GL_NO_ERROR);
	}
}

VOID OpenGLRHI::SetUniformVec3D(const GLint uniformLocation, const float x, const float y, const float z) const
{
	if (uniformLocation >= 0)
	{
		glUniform3f(uniformLocation, x, y, z);
		AssertExpr(glGetError() == GL_NO_ERROR);
	}
}

void OpenGLRHI::SetUniformVec4D(const GLint uniformLocation, const float x, const float y, const float z, const float a) const
{
	if (uniformLocation >= 0)
	{
		glUniform4f(uniformLocation, x, y, z, a);
		AssertExpr(glGetError() == GL_NO_ERROR);
	}
}

void OpenGLRHI::GenerateTexture(GLsizei numTextures, GLuint* textureID) const
{
	AssertExpr(numTextures > 0);
	glGenTextures(numTextures, textureID);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::BindTexture(GLenum target, GLuint textureID) const
{
	glBindTexture(target, textureID);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::SetTextureParami(GLenum target, GLenum paramName, GLint param) const
{
	glTexParameteri(target, paramName, param);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::TextureImage2D(
	GLenum target,
	GLint level,
	GLint internalFormat,
	GLsizei width,
	GLsizei height,
	GLint border,
	GLenum format,
	GLenum type,
	const GLvoid* data
) const
{
	glTexImage2D(target, level, internalFormat, width, height, border, format, type, data);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::SetBlendFuncParams(GLenum sourceFactor, GLenum destFactor) const
{
	glBlendFunc(sourceFactor, destFactor);
	AssertExpr(glGetError() == GL_NO_ERROR);
}

//
//	OPENGLVBO
//

OpenGLVBO::OpenGLVBO()
	: IGLBufferObject()
{
	mBufferTarget = EGLBufferTarget::ArrayBuffer;
	mBufferUsageMode = EGLBufferUsage::StaticDraw;
}

OpenGLVBO::~OpenGLVBO()
{
}

void OpenGLVBO::Init()
{
	Generate();
}

void OpenGLVBO::SetBufferUsageMode(const EGLBufferUsage::T newBufferUsageMode)
{
	mBufferUsageMode = newBufferUsageMode;
}

void OpenGLVBO::SetBufferData(const void* const data, const U32 size)
{
	OpenGLRHI::Get().SetBufferData(mBufferTarget, size, data, mBufferUsageMode);
}

void OpenGLVBO::SetBufferSubData(const void* const data, const GLsizeiptr offset, const U32 size)
{
	OpenGLRHI::Get().SetBufferSubData(mBufferTarget, offset, size, data);
}

void OpenGLVBO::Generate()
{
	OpenGLRHI::Get().GenerateBuffer(1, &mBufferHandle);
}

void OpenGLVBO::Bind()
{
	OpenGLRHI::Get().BindBuffer(mBufferTarget, mBufferHandle);
}

void OpenGLVBO::Unbind()
{
	OpenGLRHI::Get().BindBuffer(mBufferTarget, 0);
}

void OpenGLVBO::Destroy()
{
	OpenGLRHI::Get().DeleteBuffer(1, &mBufferHandle);
}

OpenGLVAO::OpenGLVAO()
	: IGLBufferObject()
{
}

//
//	OPENGLVAO
//
OpenGLVAO::~OpenGLVAO()
{
}

void OpenGLVAO::Init()
{
	Generate();
}

void OpenGLVAO::Bind()
{
	OpenGLRHI::Get().BindVertexArray(mBufferHandle);
}

void OpenGLVAO::Unbind()
{
	OpenGLRHI::Get().BindVertexArray(0);
}

void OpenGLVAO::Destroy()
{
	OpenGLRHI::Get().DeleteBuffer(1, &mBufferHandle);
}

void OpenGLVAO::Generate()
{
	OpenGLRHI::Get().GenVertexArrays(1, &mBufferHandle);
}

OpenGLEBO::OpenGLEBO()
	: IGLBufferObject()
{
	mBufferTarget = EGLBufferTarget::ElementArrayBuffer;
	mBufferUsageMode = EGLBufferUsage::StaticDraw;
}

OpenGLEBO::~OpenGLEBO()
{
	Unbind();
}

void OpenGLEBO::Init()
{
	Generate();
}

void OpenGLEBO::Bind()
{
	OpenGLRHI::Get().BindBuffer(EGLBufferTarget::ElementArrayBuffer, mBufferHandle);
}

void OpenGLEBO::Unbind()
{
	OpenGLRHI::Get().BindBuffer(EGLBufferTarget::ElementArrayBuffer, 0);
}

void OpenGLEBO::Destroy()
{
	OpenGLRHI::Get().DeleteBuffer(1, &mBufferHandle);
}

void OpenGLEBO::SetBufferData(const void* const data, const U32 size)
{
	OpenGLRHI::Get().SetBufferData(mBufferTarget, size, data, mBufferUsageMode);
}

void OpenGLEBO::SetBufferUsageMode(const EGLBufferUsage::T newBufferUsageMode)
{
	mBufferUsageMode = newBufferUsageMode;
}

void OpenGLEBO::Generate()
{
	OpenGLRHI::Get().GenerateBuffer(1, &mBufferHandle);
}
