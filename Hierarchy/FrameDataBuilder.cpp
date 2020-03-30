#include "FrameDataBuilder.h"

using FilterFunc = std::function<bool(const framedata::FrameMaterialPtr &)>;

namespace hierarchy
{

class FrameDataBuilderImpl
{
    const Scene &m_scene;

public:
    FrameDataBuilderImpl(const Scene &scene)
        : m_scene(scene)
    {
    }

    void UpdateFrameData(framedata::FrameData *framedata)
    {
        //
        // FrameData::Meshlist
        //
        for (auto &node : m_scene.gizmoNodes)
        {
            TraverseMesh(framedata, node);
        }
        if (m_scene.model)
        {
            TraverseMesh(framedata, m_scene.model->root);
        }

        //
        // FrameData::Drawlist
        //
        // Opaque
        UpdateFrameDataIf(framedata, &m_scene, [](const framedata::FrameMaterialPtr &m) {
            return m->AlphaMode != framedata::AlphaMode::Blend;
        });

        // AlphaBlend
        UpdateFrameDataIf(framedata, &m_scene, [](const framedata::FrameMaterialPtr &m) {
            return m->AlphaMode == framedata::AlphaMode::Blend;
        });
    }

private:
    void TraverseMesh(framedata::FrameData *framedata, const std::shared_ptr<SceneNode> &node)
    {
        auto mesh = node->Mesh();
        if (mesh)
        {
            framedata->Meshlist.push_back({
                .Mesh = mesh,
            });
            if (node->skin)
            {
                framedata->Meshlist.back().Skin = {
                    node->skin->cpuSkiningBuffer.data(),
                    (uint32_t)node->skin->cpuSkiningBuffer.size(),
                    mesh->vertices->stride,
                };
            }
        }
        int count;
        auto child = node->GetChildren(&count);
        for (int i = 0; i < count; ++i, ++child)
        {
            TraverseMesh(framedata, *child);
        }
    }

    void UpdateFrameDataIf(framedata::FrameData *framedata,
                           const Scene *scene,
                           const FilterFunc &filter)
    {
        if (framedata->ShowGrid)
        {
            for (auto &node : scene->gizmoNodes)
            {
                TraverseSubmesh(framedata, node, filter);
            }
        }
        if (scene->model)
        {
            TraverseSubmesh(framedata, scene->model->root, filter);
        }
    }

    void TraverseSubmesh(framedata::FrameData *framedata, const std::shared_ptr<SceneNode> &node, const FilterFunc &filter)
    {
        auto mesh = node->Mesh();
        if (mesh)
        {
            for (auto &submesh : mesh->submeshes)
            {
                auto &material = submesh.material;
                if (filter(material))
                {
                    auto vs = material->Shader ? material->Shader->VS : nullptr;
                    if (vs)
                    {
                        auto m = node->World().RowMatrix();
                        framedata::CBValue values[] = {
                            {.semantic = framedata::ConstantSemantics::NODE_WORLD,
                             .p = &m,
                             .size = sizeof(m)},
                            {.semantic = framedata::ConstantSemantics::MATERIAL_COLOR,
                             .p = material->Color.data(),
                             .size = sizeof(material->Color)},
                        };
                        framedata->PushCB(vs->DrawCB(), values, _countof(values));
                        framedata->PushDraw(mesh, submesh);
                    }
                }
            }
        }

        int count;
        auto child = node->GetChildren(&count);
        for (int i = 0; i < count; ++i, ++child)
        {
            TraverseSubmesh(framedata, *child, filter);
        }
    }
};

FrameDataBuilder::FrameDataBuilder(const Scene &scene)
    : m_impl(new FrameDataBuilderImpl(scene))
{
}

FrameDataBuilder::~FrameDataBuilder()
{
    delete m_impl;
}

void FrameDataBuilder::UpdateFrameData(framedata::FrameData *framedata)
{
    m_impl->UpdateFrameData(framedata);
}

} // namespace hierarchy