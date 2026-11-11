#pragma once

#include <Rendering/Driver/ShaderTypes.h>

struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11ComputeShader;
struct ID3D11InputLayout;

namespace Rendering
{
	class DX11Device;

	class DX11VertexShader : public IVertexShader
	{
	public:
		DX11VertexShader();
		~DX11VertexShader() override;

	public:
		// Compiles VSMain as vs_5_0. A compile error, or inputs that don't match inputLayout, is logged and leaves the
		// shader empty: its draws do nothing.
		void Create(const Filepath& filePath, const ShaderInputLayout& inputLayout) override;
		void Release() override;
		void SetActive() override;
		void SetInputLayout() override;
		bool IsCreated() const override { return m_hwShader != nullptr; }
		
	public:
		void SetDevice(DX11Device* graphicsDevice);
		

	private:
		ID3D11VertexShader* m_hwShader = nullptr;
		ID3D11InputLayout* m_hwInputLayout = nullptr;
		DX11Device* m_graphicsDevice = nullptr;
	};

	class DX11PixelShader : public IPixelShader
	{
	public:
		DX11PixelShader();
		~DX11PixelShader() override;

	public:
		// Compiles PSMain as ps_5_0. A compile error is logged and leaves the shader empty: its draws write depth only.
		void Create(const Filepath& filePath) override;
		void Release() override;
		void SetActive() override;

	public:
		void SetDevice(DX11Device* graphicsDevice);

	private:
		ID3D11PixelShader* m_hwShader = nullptr;
		DX11Device* m_graphicsDevice = nullptr;
	};

	class DX11ComputeShader : public IComputeShader
	{
	public:
		DX11ComputeShader();
		~DX11ComputeShader() override;

	public:
		// Compiles CSMain as cs_5_0. A compile error is logged and leaves the shader empty: its dispatches do nothing.
		void Create(const Filepath& filePath) override;
		void Release() override;
		void SetActive() override;
		bool IsCreated() const override { return m_hwShader != nullptr; }

	public:
		void SetDevice(DX11Device* graphicsDevice);

	private:
		ID3D11ComputeShader* m_hwShader = nullptr;
		DX11Device* m_graphicsDevice = nullptr;
	};
}