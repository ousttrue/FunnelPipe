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
                    auto shader = material->Shader;
                    if (shader)
                    {
                        auto vs = shader->VS;
                        if (vs)
                        {
                            framedata->PushCB(vs->DrawCB());
                            framedata->SetCBVariable(vs->DrawCB(), framedata::ConstantSemantics::NODE_WORLD, node->World().RowMatrix());
                            framedata->SetCBVariable(vs->DrawCB(), framedata::ConstantSemantics::MATERIAL_COLOR, material->Color);
                            framedata->SetCBVariable(vs->DrawCB(), framedata::ConstantSemantics::MATERIAL_NORMAL_SCALE, 1.0f);
                            framedata->SetCBVariable(vs->DrawCB(), framedata::ConstantSemantics::MATERIAL_EMISSIVE, material->Emissive);
                            framedata->SetCBVariable(vs->DrawCB(), framedata::ConstantSemantics::MATERIAL_OCCLUSION_STRENGTH, 1.0f);
                            framedata->SetCBVariable(vs->DrawCB(), framedata::ConstantSemantics::MATERIAL_METALLIC_ROUGHNESS, std::array<float, 2>{1, 1});
                            framedata->PushDraw(mesh, submesh);
                        }
                        /*
                        auto ps = shader->PS;
                        if(ps)                       
                        {
                            framedata->PushCB(ps->DrawCB());
                            framedata->SetCBVariable(ps->DrawCB(), framedata::ConstantSemantics::NODE_WORLD, node->World().RowMatrix());
                            framedata->SetCBVariable(ps->DrawCB(), framedata::ConstantSemantics::MATERIAL_COLOR, material->Color);
                            framedata->SetCBVariable(ps->DrawCB(), framedata::ConstantSemantics::MATERIAL_NORMAL_SCALE, 1.0f);
                            framedata->SetCBVariable(ps->DrawCB(), framedata::ConstantSemantics::MATERIAL_EMISSIVE, material->Emissive);
                            framedata->SetCBVariable(ps->DrawCB(), framedata::ConstantSemantics::MATERIAL_OCCLUSION_STRENGTH, 1.0f);
                            framedata->SetCBVariable(ps->DrawCB(), framedata::ConstantSemantics::MATERIAL_METALLIC_ROUGHNESS, std::array<float, 2>{1, 1});
                            framedata->PushDraw(mesh, submesh);
                        }
                        */
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