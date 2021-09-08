#pragma once
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>
#include <memory>
#include <gsl/span>
#include "ShaderConstantVariable.h"

namespace framedata
{

class Shader
{
protected:
    template <typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;

    std::string m_name;

    ComPtr<ID3DBlob> m_compiled;
    std::vector<ConstantBuffer> m_cblist;

    void GetConstants(const ComPtr<ID3D12ShaderReflection> &pReflection,
                      const std::string &source);

public:
    Shader(const std::string &name) : m_name(name) {}

    const std::string &Name() const { return m_name; }

    virtual bool Compile(const std::string &source,
                         const std::string &entrypoint,
                         const D3D_SHADER_MACRO *define) = 0;

    std::tuple<LPVOID, UINT> ByteCode() const
    {
        return std::make_pair(m_compiled->GetBufferPointer(),
                              static_cast<UINT>(m_compiled->GetBufferSize()));
    }

    // register(b0), register(b1), register(b2)...
    const ConstantBuffer *CB(int reg) const
    {
        for (auto &cb : m_cblist)
        {
            if (cb.reg == reg)
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
    bool Compile(const std::string &source, const std::string &entrypoint,
                 const D3D_SHADER_MACRO *define) override;
};
using PixelShaderPtr = std::shared_ptr<PixelShader>;

class VertexShader : public Shader
{
    template <typename T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    // keep semantic strings
    enum INPUT_CLASSIFICATION
    {
        INPUT_CLASSIFICATION_PER_VERTEX_DATA = 0,
        INPUT_CLASSIFICATION_PER_INSTANCE_DATA = 1
    };
    struct InputLayoutElement
    {
        LPCSTR SemanticName;
        UINT SemanticIndex;
        DXGI_FORMAT Format;
        UINT InputSlot;
        UINT AlignedByteOffset;
        INPUT_CLASSIFICATION InputSlotClass;
        UINT InstanceDataStepRate;
    };
    std::vector<std::string> m_semantics;
    std::vector<InputLayoutElement> m_layout;
    bool
    InputLayoutFromReflection(const ComPtr<ID3D12ShaderReflection> &reflection);

public:
    using Shader::Shader;
    bool Compile(const std::string &source, const std::string &entrypoint,
                 const D3D_SHADER_MACRO *define) override;
    // same with D3D11_INPUT_ELEMENT_DESC or D3D12_INPUT_ELEMENT_DESC
    gsl::span<const InputLayoutElement> InputLayout() const
    {
        return m_layout;
    }
};
using VertexShaderPtr = std::shared_ptr<VertexShader>;

} // namespace framedata
