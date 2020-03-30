#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>
#include <memory>
#include "ShaderConstantVariable.h"

namespace framedata
{

class Shader
{
protected:
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;

    std::string m_name;

    ComPtr<ID3DBlob> m_compiled;
    std::vector<ConstantBuffer> m_cblist;

    void GetConstants(const ComPtr<ID3D12ShaderReflection> &pReflection,
                      const std::string &source);

public:
    Shader(const std::string &name)
        : m_name(name)
    {
    }

    const std::string &Name() const { return m_name; }

    virtual bool Compile(const std::string &source) = 0;

    D3D12_SHADER_BYTECODE ByteCode() const
    {
        return {
            m_compiled->GetBufferPointer(),
            m_compiled->GetBufferSize(),
        };
    }

    const ConstantBuffer *DrawCB() const
    {
        for (auto &cb : m_cblist)
        {
            // register(b1)
            if (cb.reg == 1)
            {
                return &cb;
            }
        }
        return nullptr;
    }
};
using ShaderPtr = std::shared_ptr<Shader>;

class PixelShader : public Shader
{
public:
    using Shader::Shader;
    bool Compile(const std::string &source) override;
};
using PixelShaderPtr = std::shared_ptr<PixelShader>;

class VertexShader : public Shader
{
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    // keep semantic strings
    std::vector<std::string> m_semantics;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_layout;
    bool InputLayoutFromReflection(const ComPtr<ID3D12ShaderReflection> &reflection);

public:
    using Shader::Shader;
    bool Compile(const std::string &source) override;
    const D3D12_INPUT_ELEMENT_DESC *inputLayout(int *count) const
    {
        *count = (int)m_layout.size();
        return m_layout.data();
    }
};
using VertexShaderPtr = std::shared_ptr<VertexShader>;

} // namespace framedata
