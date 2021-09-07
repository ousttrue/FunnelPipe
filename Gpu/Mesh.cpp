#include "Mesh.h"
#include "CommandList.h"
#include "ResourceItem.h"

// #include "Material.h"

namespace Gpu::dx12 {

std::pair<bool, std::function<void()>>
Mesh::IsDrawable(const ComPtr<ID3D12GraphicsCommandList> &commandList) {
  //
  // vertexState
  //
  if (!m_vertexBuffer) {
    return {false, std::function<void()>()};
  }
  auto vertexState = m_vertexBuffer->State();

  std::function<void()> verticesCallback;
  if (vertexState.State == D3D12_RESOURCE_STATE_COPY_DEST) {
    if (vertexState.Upload == UploadStates::Uploaded) {
      verticesCallback = m_vertexBuffer->EnqueueTransition(
          commandList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    }
  }

  //
  // indexState
  //
  if (!m_indexBuffer) {
    throw;
  }

  std::function<void()> indicesCallback;
  auto indexState = m_indexBuffer->State();
  if (indexState.State == D3D12_RESOURCE_STATE_COPY_DEST) {
    if (indexState.Upload == UploadStates::Uploaded) {
      indicesCallback = m_indexBuffer->EnqueueTransition(
          commandList, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    }
  }

  //
  // draw Indexed
  //
  if (!vertexState.Drawable() || !indexState.Drawable()) {
    return {false, [verticesCallback, indicesCallback]() {
              if (verticesCallback) {
                verticesCallback();
              }
              if (indicesCallback) {
                indicesCallback();
              }
            }};
  }

  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  auto vb = m_vertexBuffer->VertexBufferView();
  commandList->IASetVertexBuffers(0, 1, &vb);
  auto ib = m_indexBuffer->IndexBufferView();
  commandList->IASetIndexBuffer(&ib);

  return {true, std::function<void()>()};
}

} // namespace Gpu::dx12