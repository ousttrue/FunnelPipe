#include "Shader.h"
#include <plog/log.h>
#include "DirectoryWatcher.h"
#include "ToUnicode.h"

namespace framedata
{

static std::string ToString(const Microsoft::WRL::ComPtr<ID3DBlob> &blob)
{
    std::vector<uint8_t> buffer(blob->GetBufferSize());
    memcpy(buffer.data(), blob->GetBufferPointer(), buffer.size());
    return std::string(buffer.begin(), buffer.end());
}

void Shader::GetConstants(const ComPtr<ID3D12ShaderReflection> &pReflection,
                          const std::string &source)
{
    D3D12_SHADER_DESC desc;
    pReflection->GetDesc(&desc);

    for (unsigned i = 0; i < desc.ConstantBuffers; ++i)
    {
        auto cb = pReflection->GetConstantBufferByIndex(i);
        m_cblist.push_back({});
        m_cblist.back().GetVariables(pReflection.Get(), cb, source);
    }
}

bool VertexShader::InputLayoutFromReflection(const ComPtr<ID3D12ShaderReflection> &pReflection)
{
    // Define the vertex input layout.
    // D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    //     {
    //         {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    //         {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    //         {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    //     };
    // auto inputElementDescs = InputLayoutFromReflection(vertexShader);

    D3D12_SHADER_DESC desc;
    pReflection->GetDesc(&desc);
    m_semantics.reserve(desc.InputParameters);
    for (unsigned i = 0; i < desc.InputParameters; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC lParamDesc;
        pReflection->GetInputParameterDesc(i, &lParamDesc);

        m_semantics.push_back(lParamDesc.SemanticName);
        D3D12_INPUT_ELEMENT_DESC lElementDesc{
            .SemanticName = m_semantics.back().c_str(),
            .SemanticIndex = lParamDesc.SemanticIndex,
            .InputSlot = 0,
            .AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
            .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            .InstanceDataStepRate = 0,
        };

        if (lParamDesc.Mask == 1)
        {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if (lParamDesc.Mask <= 3)
        {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if (lParamDesc.Mask <= 7)
        {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (lParamDesc.Mask <= 15)
        {
            if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (lParamDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
                lElementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        else
        {
            throw "unknown";
        }

        m_layout.push_back(lElementDesc);
    }

    return true;
}

char INCLUDE[] = R"(cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 b0View : CAMERA_VIEW;
    float4x4 b0Projection : CAMERA_PROJECTION;
    float3 b0LightDirection : LIGHT_DIRECTION;
    float3 b0LightColor : LIGHT_COLOR;
};
)";

struct Buffer
{
    std::string data;
};
class IncludeHandler : public ID3DInclude
{
    std::vector<std::shared_ptr<Buffer>> m_buffers;

    HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) override
    {
        switch (IncludeType)
        {
        case D3D_INCLUDE_LOCAL:
        case D3D_INCLUDE_SYSTEM:
            break;

        default:
            return E_FAIL;
        }
        auto watcher = DirectoryWatcher::Instance().Get(SJISToUnicode(pFileName));

        auto buffer = std::make_shared<Buffer>();

        m_buffers.emplace_back(buffer);

        auto copy = watcher->Copy();
        buffer->data = std::string(copy.begin(), copy.end());

        // int i = 0;
        // for (; i < buffer->data.size(); ++i)
        // {
        //     if (buffer->data[i] != INCLUDE[i])
        //     {
        //         break;
        //     }
        // }
        // auto a = 0;

        *ppData = buffer->data.data();
        *pBytes = (UINT)buffer->data.size();
        return S_OK;
    }

    HRESULT Close(LPCVOID pData) override
    {
        return S_OK;
    }
};

bool VertexShader::Compile(const std::string &source, const std::string &entrypoint, const D3D_SHADER_MACRO *define)
{
    if (source.empty())
    {
        return false;
    }

    // Create the pipeline state, which includes compiling and loading shaders.
#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    IncludeHandler includeHandler;

    //
    // VS
    //
    ComPtr<ID3DBlob> error;
    if (FAILED(D3DCompile(source.data(), source.size(), m_name.c_str(),
                          define, &includeHandler,
                          entrypoint.c_str(), "vs_5_0", compileFlags, 0, &m_compiled, &error)))
    {
        if (error)
        {
            LOGW << ToString(error);
        }
        else
        {
            LOGW << "unknown error";
        }
        return false;
    }
    ComPtr<ID3D12ShaderReflection> pReflection;
    if (FAILED(D3DReflect(m_compiled->GetBufferPointer(), m_compiled->GetBufferSize(), IID_PPV_ARGS(&pReflection))))
    {
        return false;
    }
    if (!InputLayoutFromReflection(pReflection))
    {
        return false;
    }
    GetConstants(pReflection, source);

    return true;
}

bool PixelShader::Compile(const std::string &source, const std::string &entrypoint, const D3D_SHADER_MACRO *define)
{
    // Create the pipeline state, which includes compiling and loading shaders.
#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    IncludeHandler includeHandler;

    ComPtr<ID3DBlob> error;
    if (FAILED(D3DCompile(source.data(), source.size(), m_name.c_str(),
                          define, &includeHandler,
                          entrypoint.c_str(), "ps_5_0", compileFlags, 0, &m_compiled, nullptr)))
    {
        if (error)
        {
            LOGW << ToString(error);
        }
        else
        {
            LOGW << "unknown error";
        }
        return false;
    }
    ComPtr<ID3D12ShaderReflection> pReflection;
    if (FAILED(D3DReflect(m_compiled->GetBufferPointer(), m_compiled->GetBufferSize(), IID_PPV_ARGS(&pReflection))))
    {
        return false;
    }
    GetConstants(pReflection, source);

    return true;
}

} // namespace framedata
