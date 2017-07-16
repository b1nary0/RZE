#include "StdAfx.h"

#include "OpenGL.h"

OpenGLRHI::OpenGLRHI()
{
}

OpenGLRHI::~OpenGLRHI()
{
}

void OpenGLRHI::Init(const OpenGLCreationParams& creationParams)
{
    LOG_CONSOLE_ARGS("OpenGL version is %s", glGetString(GL_VERSION));

    // @implementation these need to be read in via window attributes
    glViewport(0, 0, creationParams.WindowWidth, creationParams.WindowHeight);

    InitGLEW();
}

void OpenGLRHI::InitGLEW()
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

void OpenGLRHI::EnableCapability(const GLuint capability)
{
    glEnable(capability);
    AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::LogShaderInfo(const GLuint shaderProgramID)
{
    GLint length;
    GetShaderiv(shaderProgramID, GL_INFO_LOG_LENGTH, &length);

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

void OpenGLRHI::GenerateBuffer(GLuint bufferCount, GLuint* outBufferHandle) const
{
    AssertExpr(bufferCount > 0);
    glGenBuffers(bufferCount, outBufferHandle);
    AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::BindBuffer(const GLuint target, const GLuint bufferObjectHandle) const
{
    glBindBuffer(target, bufferObjectHandle);
    AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::DeleteBuffer(GLuint bufferCount, GLuint* bufferHandle)
{
    AssertExpr(bufferCount > 0 && bufferHandle);
    glDeleteBuffers(bufferCount, bufferHandle);
    AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::SetBufferData(const GLuint target, const GLuint size, const void* const data, const GLuint bufferUsage) const
{
    AssertNotNull(data);
    AssertExpr(size > 0);
    // @note if this doesnt work check the const void* const signature because its possible under the hood it no likey
    glBufferData(target, size, data, bufferUsage);
    glGetError();
    AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::EnableVertexAttributeArray(const GLuint index) const
{
    glEnableVertexAttribArray(index);
    AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::VertexAttribPointer(const GLuint index, const GLint size, const GLuint type, const GLboolean normalized, const GLuint stride, const void* const pointer) const
{
    AssertExpr(size > 0);
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::DrawArrays(const GLuint mode, const GLint first, const GLuint count) const
{
    AssertExpr(count > 0);
    glDrawArrays(mode, first, count);
}

void OpenGLRHI::DrawElements(const GLuint mode, const GLsizei count, GLenum type, const GLvoid* indices)
{
    AssertExpr(count > 0);
    glDrawElements(mode, count, type, indices);
}

void OpenGLRHI::CreateShaderProgram(GLuint& outProgramID) const
{
    outProgramID = glCreateProgram();
    AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::CreateShader(const GLuint shaderType, GLuint& outShaderID) const
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

void OpenGLRHI::GetProgramiv(const GLuint mShaderProgramID, const EGLShaderProgramStatusParam::Value programStatusParam, GLint* programLinkStatus) const
{
    glGetProgramiv(mShaderProgramID, programStatusParam, programLinkStatus);
    AssertExpr(glGetError() == GL_NO_ERROR);
}

void OpenGLRHI::GetShaderiv(const GLuint shaderID, const GLenum shaderStatusParam, GLint* const params) const
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

//
//	OPENGLVBO
//

OpenGLVBO::OpenGLVBO(OpenGLVAO* parentBuf)
    : IGLBufferObject()
{
    mBufferTarget = EGLBufferTarget::ArrayBuffer;
    mBufferUsageMode = EGLBufferUsage::StaticDraw;

    parentBuf->Bind();

    Generate();
    Bind();
    
    parentBuf->Unbind();
}

OpenGLVBO::~OpenGLVBO()
{
}

void OpenGLVBO::SetBufferTarget(const U32 newBufferTarget)
{
    mBufferTarget = newBufferTarget;
}

void OpenGLVBO::SetBufferUsageMode(const U32 newBufferUsageMode)
{
    mBufferUsageMode = newBufferUsageMode;
}

void OpenGLVBO::SetBufferData(const void* const data, const U32 size)
{
    AssertNotNull(data);
    OpenGLRHI::Get().SetBufferData(mBufferTarget, size, data, mBufferUsageMode);
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
    Generate();
}

//
//	OPENGLVAO
//
OpenGLVAO::~OpenGLVAO()
{

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

OpenGLEBO::OpenGLEBO(OpenGLVAO* parentBuf)
    : IGLBufferObject()
{
    mBufferTarget = EGLBufferTarget::ElementArrayBuffer;
    mBufferUsageMode = EGLBufferUsage::StaticDraw;

    parentBuf->Bind();

    Generate();
    Bind();

    parentBuf->Unbind();
}

OpenGLEBO::~OpenGLEBO()
{
    Unbind();
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

void OpenGLEBO::SetBufferTarget(const U32 newBufferTarget)
{
    mBufferTarget = newBufferTarget;
}

void OpenGLEBO::SetBufferData(const void* const data, const U32 size)
{
    AssertNotNull(data);
    OpenGLRHI::Get().SetBufferData(mBufferTarget, size, data, mBufferUsageMode);
}

void OpenGLEBO::SetBufferUsageMode(const U32 newBufferUsageMode)
{
    mBufferUsageMode = newBufferUsageMode;
}

void OpenGLEBO::Generate()
{
    OpenGLRHI::Get().GenerateBuffer(1, &mBufferHandle);
}
