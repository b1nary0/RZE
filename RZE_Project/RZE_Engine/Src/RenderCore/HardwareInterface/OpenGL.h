#pragma once

#include "Utils/PrimitiveDefs.h"

struct EGLBooleanValue
{
	enum Value : GLboolean
	{
		True = GL_TRUE,
		False = GL_FALSE
	};
};

struct EGLBufferBit
{
	enum Value : GLbitfield
	{
		Accumulation = GL_ACCUM_BUFFER_BIT,
		Color = GL_COLOR_BUFFER_BIT,
		Depth = GL_DEPTH_BUFFER_BIT,
		Stencil = GL_STENCIL_BUFFER_BIT
	};
};

struct EGLBufferTarget
{
	enum Value : GLenum
	{
		ArrayBuffer = GL_ARRAY_BUFFER,
		AtomicCounterBuffer = GL_ATOMIC_COUNTER_BUFFER,
		CopyReadBuffer = GL_COPY_READ_BUFFER,
		CopyWriteBuffer = GL_COPY_WRITE_BUFFER,
		DispatchIndirectBuffer = GL_DISPATCH_INDIRECT_BUFFER,
		DrawIndirectBuffer = GL_DRAW_INDIRECT_BUFFER,
		ElementArrayBuffer = GL_ELEMENT_ARRAY_BUFFER,
		PixelPackBuffer = GL_PIXEL_PACK_BUFFER,
		PixelUnpackBuffer = GL_PIXEL_UNPACK_BUFFER,
		QueryBuffer = GL_QUERY_BUFFER,
		ShaderStorageBuffer = GL_SHADER_STORAGE_BUFFER,
		TextureBuffer = GL_TEXTURE_BUFFER,
		TransformFeedbackBuffer = GL_TRANSFORM_FEEDBACK_BUFFER,
		UniformBuffer = GL_UNIFORM_BUFFER
	};
};

struct EGLBufferUsage
{
	enum Value : GLenum
	{
		StreamCopy = GL_STREAM_COPY,
		StreamDraw = GL_STREAM_DRAW,
		StreamRead = GL_STREAM_READ,

		StaticCopy = GL_STATIC_COPY,
		StaticDraw = GL_STATIC_DRAW,
		StaticRead = GL_STATIC_READ,

		DynamicCopy = GL_DYNAMIC_COPY,
		DynamicDraw = GL_DYNAMIC_DRAW,
		DynamicRead = GL_DYNAMIC_READ
	};
};

struct EGLCapability
{
	enum Value : GLenum
	{
		AlphaTest = GL_ALPHA_TEST,
		GenerateNormals = GL_AUTO_NORMAL,
		Blend = GL_BLEND,
		ClipPlane0 = GL_CLIP_PLANE0,
		ClipPlane1 = GL_CLIP_PLANE1,
		ClipPlane2 = GL_CLIP_PLANE2,
		ClipPlane3 = GL_CLIP_PLANE3,
		ClipPlane4 = GL_CLIP_PLANE4,
		ClipPlane5 = GL_CLIP_PLANE5,
		ColorLogicOperation = GL_COLOR_LOGIC_OP,
		ColorMaterial = GL_COLOR_MATERIAL,
		ColorSum = GL_COLOR_SUM,
		ColorTable = GL_COLOR_TABLE,
		Convolution1D = GL_CONVOLUTION_1D,
		Convolution2D = GL_CONVOLUTION_2D,
		CullFace = GL_CULL_FACE,
		DepthTest = GL_DEPTH_TEST,
		Dither = GL_DITHER,
		Fog = GL_FOG,
		Histogram = GL_HISTOGRAM,
		IndexLogicOperation = GL_INDEX_LOGIC_OP,
		Lighting = GL_LIGHTING,
		Light0 = GL_LIGHT0,
		Light1 = GL_LIGHT1,
		Light2 = GL_LIGHT2,
		Light3 = GL_LIGHT3,
		LineSmooth = GL_LINE_SMOOTH,
		LineStipple = GL_LINE_STIPPLE,
		Map1Color4 = GL_MAP1_COLOR_4,
		Map1Index = GL_MAP1_INDEX,
		Map1Normal = GL_MAP1_NORMAL,
		Map1TextureCoord1 = GL_MAP1_TEXTURE_COORD_1,
		Map1TextureCoord2 = GL_MAP1_TEXTURE_COORD_2,
		Map1TextureCoord3 = GL_MAP1_TEXTURE_COORD_3,
		Map1TextureCoord4 = GL_MAP1_TEXTURE_COORD_4,
		Map1Vertex3 = GL_MAP1_VERTEX_3,
		Map1Vertex4 = GL_MAP1_VERTEX_4,
		Map2Color4 = GL_MAP2_COLOR_4,
		Map2Index = GL_MAP2_INDEX,
		Map2Normal = GL_MAP2_NORMAL,
		Map2TextureCoord1 = GL_MAP2_TEXTURE_COORD_1,
		Map2TextureCoord2 = GL_MAP2_TEXTURE_COORD_2,
		Map2TextureCoord3 = GL_MAP2_TEXTURE_COORD_3,
		Map2TextureCoord4 = GL_MAP2_TEXTURE_COORD_4,
		Map2Vertex3 = GL_MAP2_VERTEX_3,
		Map2Vertex4 = GL_MAP2_VERTEX_4,
		MinMax = GL_MINMAX,
		MultiSample = GL_MULTISAMPLE,
		Normalize = GL_NORMALIZE,
		PointSmooth = GL_POINT_SMOOTH,
		PointSprite = GL_POINT_SPRITE,
		PolygonOffsetFill = GL_POLYGON_OFFSET_FILL,
		PolygonOffsetLine = GL_POLYGON_OFFSET_LINE,
		PolygonOffsetPoint = GL_POLYGON_OFFSET_POINT,
		PolygonSmooth = GL_POLYGON_SMOOTH,
		PolygonStipple = GL_POLYGON_STIPPLE,
		RescaleNormal = GL_RESCALE_NORMAL,
		SampleAlphaToCoverage = GL_SAMPLE_ALPHA_TO_COVERAGE,
		SampleAlphaToOne = GL_SAMPLE_ALPHA_TO_ONE,
		SampleCoverage = GL_SAMPLE_COVERAGE,
		Separable2D = GL_SEPARABLE_2D,
		ScissorTest = GL_SCISSOR_TEST,
		StencilTest = GL_STENCIL_TEST,
		Texture1D = GL_TEXTURE_1D,
		Texture2D = GL_TEXTURE_2D,
		Texture3D = GL_TEXTURE_3D,
		TextureCubeMap = GL_TEXTURE_CUBE_MAP,
		TextureGenQ = GL_TEXTURE_GEN_Q,
		TextureGenR = GL_TEXTURE_GEN_R,
		TextureGenS = GL_TEXTURE_GEN_S,
		TextureGenT = GL_TEXTURE_GEN_T,
		VertexProgramPointSize = GL_VERTEX_PROGRAM_POINT_SIZE,
		VertexProgramTwoSide = GL_VERTEX_PROGRAM_TWO_SIDE
	};
};

struct EGLDataType
{
	enum Value : GLenum
	{
		Byte = GL_BYTE,
		Double = GL_DOUBLE,
		Fixed = GL_FIXED,
		Float = GL_FLOAT,
		HalfFloat = GL_HALF_FLOAT,
		Int = GL_INT,
		Short = GL_SHORT,
		UnsignedByte = GL_UNSIGNED_BYTE,
		UnsignedInt = GL_UNSIGNED_INT,
		UnsignedShort = GL_UNSIGNED_SHORT
	};
};

struct EGLDrawMode
{
	enum Value : GLenum
	{
		Points = GL_POINTS,

		LineStrip = GL_LINE_STRIP,
		LineLoop = GL_LINE_LOOP,
		Lines = GL_LINES,
		LineStripAdjacency = GL_LINE_STRIP_ADJACENCY,

		TriangleStrip = GL_TRIANGLE_STRIP,
		TriangleFan = GL_TRIANGLE_FAN,
		Triangles = GL_TRIANGLES,
		TriangleStripAdjacency = GL_TRIANGLE_STRIP_ADJACENCY,
		TrianglesAdjacency = GL_TRIANGLES_ADJACENCY,

		Patches = GL_PATCHES
	};
};

struct EGLPolygonMode
{
	enum Value : GLenum
	{
		Point = GL_POINT,
		Line = GL_LINE,
		Fill = GL_FILL
	};
};

struct EGLPolygonModeFace
{
	enum Value : GLenum
	{
		Front = GL_FRONT,
		Back = GL_BACK,
		FrontAndBack = GL_FRONT_AND_BACK
	};
};

struct EGLShaderType
{
	enum Value : GLenum
	{
		Compute = GL_COMPUTE_SHADER,
		Vertex = GL_VERTEX_SHADER,
		TessControl = GL_TESS_CONTROL_SHADER,
		TessEval = GL_TESS_EVALUATION_SHADER,
		Geometry = GL_GEOMETRY_SHADER,
		Fragment = GL_FRAGMENT_SHADER
	};
};

struct EGLShaderProgramStatusParam
{
	enum Value : GLenum
	{
		DeleteStatus = GL_DELETE_STATUS,
		LinkStatus = GL_LINK_STATUS,
		ValidateStatus = GL_VALIDATE_STATUS,
		InfoLogLength = GL_INFO_LOG_LENGTH,
		AttachedShaders = GL_ATTACHED_SHADERS,
		ActiveAttributes = GL_ACTIVE_ATTRIBUTES,
		ActiveAttributeMaxLength = GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,
		ActiveUniforms = GL_ACTIVE_UNIFORMS,
		ActiveUniformMaxLength = GL_ACTIVE_UNIFORM_MAX_LENGTH
	};
};

struct EGLShaderStatusParam
{
	enum Value : GLenum
	{
		ShaderType = GL_SHADER_TYPE,
		DeleteStatus = GL_DELETE_STATUS,
		CompileStatus = GL_COMPILE_STATUS,
		InfoLogLength = GL_INFO_LOG_LENGTH,
		ShaderSourceLength = GL_SHADER_SOURCE_LENGTH
	};
};

struct OpenGLContext
{
	HWND windowHandle;
	HGLRC openGLContext;
	HDC deviceContext;
};

class OpenGLVBO
{
public:
	OpenGLVBO();
	//OpenGLVBO(const UInt32 count);
	~OpenGLVBO();

	void ClearAndRegenBufferCount(const U32 newCount);
	void SetBufferTarget(const U32 newBufferTarget);
	void SetBufferUsageMode(const U32 newBufferUsageMode);
	void SetBufferData(const void* const data, const U32 size);

private:
	void GenerateBuffers(const U32 count);
	void BindBuffers(const U32 index);

	U32 mBufferUsageMode;
	U32 mBufferTarget;
	U32 mBufferCount;
	U32 mBufferHandle;
};

class OpenGLRHI
{
public:
	OpenGLRHI();
	~OpenGLRHI();

	static OpenGLRHI& Get()
	{
		static OpenGLRHI instance;
		return instance;
	}

	void Init();

	void ClearColor(const GLfloat red, const  GLfloat green, const  GLfloat blue, const  GLfloat alpha) const;
	void Clear(const GLuint mask) const;

	// Returns the array object  handle ID generated
	void GenVertexArrays(const GLuint arrayCount, GLuint* outBufferHandle) const;
	void BindVertexArray(const GLuint arrayObjectHandle) const;

	void GenerateBuffer(GLuint bufferCount, GLuint* outBufferHandle) const;
	void BindBuffer(const GLuint target, const GLuint bufferObjectHandle) const;
	void DeleteBuffer(GLuint bufferCount, GLuint* bufferHandle);
	void SetBufferData(const GLuint target, const GLuint size, const void* const data, const GLuint bufferUsage) const;

	void EnableVertexAttributeArray(const GLuint index) const;
	void VertexAttribPointer(const GLuint index, const GLint size, const GLuint type, const GLboolean normalized, const GLuint stride, const void* const pointer) const;

	void DrawArrays(const GLuint mode, const GLint first, const GLuint count) const;

	//
	// Shaders
	//
	void CreateShaderProgram(GLuint& outProgramID) const;
	void CreateShader(const GLuint shaderType, GLuint& outShaderID) const;
	void CompileShader(const GLuint shaderID) const;
	void AttachShader(const GLuint shaderProgramID, const GLuint shaderID) const;
	void UseShaderProgram(const GLuint shaderProgramID) const;
	void SetShaderSource(const GLuint shaderID, const GLuint count, const GLchar* sourceCode, const GLint* length) const;
	void IsShader(const GLuint shaderID, GLboolean& outResult) const;
	void IsShaderProgram(const GLuint shaderProgramID, GLboolean& outResult) const;
	void LinkShaderProgram(const GLuint mShaderProgramID) const;
	void GetProgramiv(const GLuint shaderProgramID, const EGLShaderProgramStatusParam::Value programStatusParam, GLint* programLinkStatus) const;
	void UniformLocation(const GLuint shaderProgramID, const GLchar* uniformName, const GLint uniformLocation);

private:

	void InitGLEW();
};
