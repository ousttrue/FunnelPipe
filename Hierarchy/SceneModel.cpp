#include "SceneModel.h"
#undef OPAQUE // wingdi.h
#include <gltfformat/glb.h>
#include <gltfformat/bin.h>
#include "ParseGltf.h"
#include "SceneMeshSkin.h"
#include "ToUnicode.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <plog/Log.h>
#include <FrameData.h>

struct GltfVertex
{
    std::array<float, 3> position;
    std::array<float, 3> normal;
    std::array<float, 2> uv;
};
static_assert(sizeof(GltfVertex) == 32, "GltfVertex size");

template <class T>
static std::vector<uint8_t> read_allbytes(T path)
{
    std::vector<uint8_t> buffer;

    // open the file for binary reading
    std::ifstream file;
    file.open(path, std::ios_base::binary);
    if (file.is_open())
    {
        // get the length of the file
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // read the file
        buffer.resize(fileSize);
        file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
    }

    return buffer;
}

static bool IsUnlit(const gltfformat::Material &gltfMatrial)
{
    if (!gltfMatrial.extensions.has_value())
    {
        return false;
    }
    if (!gltfMatrial.extensions.value().KHR_materials_unlit.has_value())
    {
        return false;
    }
    return true;
}

template <size_t N>
static void SetIf(std::array<float, N> &dst, const std::vector<float> &src)
{
    if (src.size() == dst.size())
    {
        for (int i = 0; i < dst.size(); ++i)
        {
            dst[i] = src[i];
        }
    }
}

namespace hierarchy
{

class GltfLoader
{
    const gltfformat::glTF &m_gltf;
    gltfformat::bin m_bin;

    SceneModelPtr m_model;

    struct GltfPrimitive
    {
        framedata::FrameMeshPtr mesh;
        std::vector<VertexSkining> skining;

        void LoadVertices(
            const gltfformat::glTF &gltf,
            const gltfformat::bin &bin,
            const gltfformat::Mesh &gltfMesh,
            const gltfformat::MeshPrimitive &gltfPrimitive)
        {
            mesh = framedata::FrameMesh::Create(gltfMesh.name);

            std::vector<GltfVertex> vertices;
            int vertexCount = 0;
            for (auto [k, v] : gltfPrimitive.attributes)
            {
                auto accessor = gltf.accessors[v];
                auto [p, size] = bin.get_bytes(accessor);
                auto count = accessor.count.value();
                vertices.resize(count);
                if (k == "POSITION")
                {
                    vertexCount = count;
                    for (auto i = 0; i < count; ++i, p += 12)
                    {
                        vertices[i].position = *(falg::float3 *)p;
                    }
                }
                else if (k == "NORMAL")
                {
                    for (auto i = 0; i < count; ++i, p += 12)
                    {
                        vertices[i].normal = *(falg::float3 *)p;
                    }
                }
                else if (k == "TEXCOORD_0")
                {
                    for (auto i = 0; i < count; ++i, p += 8)
                    {
                        vertices[i].uv = *(falg::float2 *)p;
                    }
                }
                else if (k == "JOINTS_0")
                {
                    skining.resize(count);
                    switch (accessor.componentType.value())
                    {
                    case gltfformat::AccessorComponentType::BYTE:
                    {
                        for (auto i = 0; i < count; ++i, p += 4)
                        {
                            auto &joints = *(std::array<uint8_t, 4> *)p;
                            skining[i].joints[0] = joints[0];
                            skining[i].joints[1] = joints[1];
                            skining[i].joints[2] = joints[2];
                            skining[i].joints[3] = joints[3];
                        }
                        break;
                    }

                    case gltfformat::AccessorComponentType::UNSIGNED_SHORT:
                    {
                        for (auto i = 0; i < count; ++i, p += 8)
                        {
                            skining[i].joints = *(std::array<uint16_t, 4> *)p;
                        }
                    }
                    break;

                    default:
                        throw;
                    }
                }
                else if (k == "WEIGHTS_0")
                {
                    for (auto i = 0; i < count; ++i, p += 16)
                    {
                        skining[i].weights = *(std::array<float, 4> *)p;
                    }
                }
                else if (k == "TANGENT")
                {
                    // do nothing
                }
                else
                {
                    auto a = 0;
                }
            }
            assert(vertices.size() == vertexCount);
            mesh->vertices = framedata::VertexBuffer::CreateStatic(
                framedata::Semantics::Vertex,
                sizeof(GltfVertex), vertices.data(), (uint32_t)(vertices.size() * sizeof(GltfVertex)));
        }
    };
    struct GltfMeshGroup
    {
        std::string name;
        std::vector<GltfPrimitive> primitives;
    };
    std::vector<std::shared_ptr<GltfMeshGroup>> m_groups;

public:
    GltfLoader(const gltfformat::glTF &gltf, const uint8_t *p, int size)
        : m_gltf(gltf), m_bin(gltf, p, size), m_model(new SceneModel)
    {
    }

    void LoadTextures()
    {
        m_model->textures.reserve(m_gltf.textures.size());
        for (auto &gltfTexture : m_gltf.textures)
        {
            auto &gltfImage = m_gltf.images[gltfTexture.source.value()];
            auto &bufferView = m_gltf.bufferViews[gltfImage.bufferView.value()];
            auto bytes = m_bin.get_bytes(bufferView);

            auto texture = std::make_shared<framedata::FrameTexture>();
            texture->Name = gltfTexture.name;

            // TO_PNG
            texture->Images.push_back(framedata::FrameImage::Load(bytes.p, bytes.size));
            texture->Images.back()->name = gltfImage.name;

            // TODO: sampler

            m_model->textures.push_back(texture);
        }
    }

    template <typename T>
    void SetTextureOr(framedata::FrameTexturePtr &dst, const std::optional<T> &src,
                      const framedata::FrameTexturePtr &value) const
    {
        if (src.has_value())
        {
            auto &gltfTexture = m_gltf.textures[src.value().index.value()];
            auto texture = m_model->textures[gltfTexture.source.value()];
            dst = texture;
        }
        else
        {
            dst = value;
        }
    }

    void LoadMaterials()
    {
        m_model->materials.reserve(m_gltf.materials.size());
        for (auto &gltfMaterial : m_gltf.materials)
        {
            auto material = std::make_shared<framedata::FrameMaterial>();
            material->Name = gltfMaterial.name;
            material->Shader =
                IsUnlit(gltfMaterial)
                    ? framedata::ShaderManager::Instance().GltfUnlit()
                    : framedata::ShaderManager::Instance().GltfPBR();
            material->DoubleSided = gltfMaterial.doubleSided.value_or(false);
            switch (gltfMaterial.alphaMode.value_or(gltfformat::MaterialAlphaMode::OPAQUE))
            {
            case gltfformat::MaterialAlphaMode::OPAQUE:
                material->AlphaMode = framedata::AlphaMode::Opaque;
                break;
            case gltfformat::MaterialAlphaMode::MASK:
                material->AlphaMode = framedata::AlphaMode::Mask;
                break;
            case gltfformat::MaterialAlphaMode::BLEND:
                material->AlphaMode = framedata::AlphaMode::Blend;
                break;
            default:
                throw "unknown";
            }

            //
            // pbrMetallicRoughness
            //
            if (gltfMaterial.pbrMetallicRoughness.has_value())
            {
                auto &pbr = gltfMaterial.pbrMetallicRoughness.value();
                SetTextureOr(material->ColorTexture, pbr.baseColorTexture, framedata::FrameTexture::One());
                SetIf(material->Color, pbr.baseColorFactor);
                SetTextureOr(material->MetallicRoughnessTexture, pbr.metallicRoughnessTexture, framedata::FrameTexture::One());
                material->Metallic = pbr.metallicFactor.value_or(1.0f);
                material->Roughness = pbr.roughnessFactor.value_or(1.0f);
            }

            SetTextureOr(material->NormalTexture, gltfMaterial.normalTexture, nullptr);
            SetTextureOr(material->OcclusionTexture, gltfMaterial.occlusionTexture, nullptr);
            SetTextureOr(material->EmissiveTexture, gltfMaterial.emissiveTexture, framedata::FrameTexture::Zero());
            SetIf(material->Emissive, gltfMaterial.emissiveFactor);

            m_model->materials.push_back(material);
        }
    }

    void LoadNodes()
    {
        int i = 0;
        m_model->nodes.reserve(m_gltf.nodes.size());
        for (auto &gltfNode : m_gltf.nodes)
        {
            auto name = gltfNode.name;
            if (name.empty())
            {
                std::stringstream ss;
                ss << "node#" << i;
                name = ss.str();
            }
            auto node = SceneNode::Create(name);

            if (gltfNode.matrix.size() == 16)
            {
                auto trs = falg::RowMatrixDecompose(falg::vector_cast<falg::float16>(gltfNode.matrix));
                node->Local(trs.transform);

                // throw("not implemented");
                auto length = falg::Length(node->Local().rotation);
                auto delta = abs(1 - length);
                if (delta > 1e-5f)
                {
                    throw;
                }
            }
            else
            {
                if (gltfNode.translation.size() == 3)
                {
                    node->Local().translation = falg::vector_cast<falg::float3>(gltfNode.translation);
                }
                if (gltfNode.rotation.size() == 4)
                {
                    node->Local().rotation = falg::vector_cast<falg::float4>(gltfNode.rotation);
                    auto length = falg::Length(node->Local().rotation);
                    auto delta = abs(1 - length);
                    if (delta > 1e-5f)
                    {
                        throw;
                    }
                }
                if (gltfNode.scale.size() == 3)
                {
                    // node->TRS.scale = falg::vector_cast<falg::float3>(gltfNode.scale);
                    // throw("not implemented");
                }
            }
            // node->EnableGizmo(true);
            m_model->nodes.push_back(node);
            ++i;
        }
    }

    bool HasSharedAccessorAttributes(const gltfformat::Mesh &gltfMesh)
    {
        auto &first = gltfMesh.primitives.front().attributes;
        for (size_t i = 1; i < gltfMesh.primitives.size(); ++i)
        {
            auto &current = gltfMesh.primitives[i].attributes;
            if (first.size() != current.size())
            {
                return false;
            }
            for (auto [k, v] : current)
            {
                auto found = first.find(k);
                if (found == first.end())
                {
                    return false;
                }
                if (found->second != v)
                {
                    return false;
                }
            }
        }
        return true;
    }

    void LoadMeshes()
    {
        m_groups.reserve(m_gltf.meshes.size());
        for (auto &gltfMesh : m_gltf.meshes)
        {
            auto shared = HasSharedAccessorAttributes(gltfMesh);
            if (shared)
            {
                m_groups.push_back(LoadSharedPrimitives(gltfMesh));
            }
            else
            {
                m_groups.push_back(LoadIsolatedPrimitives(gltfMesh));
            }
        }
    }

    std::shared_ptr<framedata::VertexBuffer> LoadIndices(const gltfformat::glTF &gltf,
                                                         const gltfformat::bin &bin,
                                                         const gltfformat::MeshPrimitive &gltfPrimitive)
    {
        if (!gltfPrimitive.indices.has_value())
        {
            return nullptr;
        }

        auto index = gltfPrimitive.indices.value();
        auto accessor = gltf.accessors[index];
        auto [p, size] = bin.get_bytes(accessor);
        int stride = 0;
        switch (accessor.componentType.value())
        {
        case gltfformat::AccessorComponentType::UNSIGNED_SHORT:
        case gltfformat::AccessorComponentType::SHORT:
            stride = 2;
            break;

        case gltfformat::AccessorComponentType::UNSIGNED_INT:
            stride = 4;
            break;

        default:
            throw;
        }

        return framedata::VertexBuffer::CreateStatic(
            framedata::Semantics::Index, stride, p, size);
    }

    std::shared_ptr<GltfMeshGroup> LoadSharedPrimitives(const gltfformat::Mesh &gltfMesh)
    {
        auto group = std::make_shared<GltfMeshGroup>();
        group->name = gltfMesh.name;
        group->primitives.push_back({});
        auto &prim = group->primitives.back();

        // shared vertices
        prim.LoadVertices(m_gltf, m_bin, gltfMesh, gltfMesh.primitives.front());

        uint32_t offset = 0;
        for (auto &gltfPrimitive : gltfMesh.primitives)
        {
            if (!prim.mesh->indices)
            {
                prim.mesh->indices = LoadIndices(m_gltf, m_bin, gltfPrimitive);
            }
            else
            {
                // merge index buffer
                prim.mesh->indices->Append(LoadIndices(m_gltf, m_bin, gltfPrimitive));
            }
            {
                // each primitive to submesh
                auto index = gltfPrimitive.indices.value();
                auto accessor = m_gltf.accessors[index];

                auto material = m_model->materials[gltfPrimitive.material.value()];
                prim.mesh->submeshes.push_back({
                    .drawOffset = offset,
                    .drawCount = (uint32_t)accessor.count.value(),
                    .material = material,
                });
                offset += accessor.count.value();
            }
        }
        return group;
    }

    std::shared_ptr<GltfMeshGroup> LoadIsolatedPrimitives(const gltfformat::Mesh &gltfMesh)
    {
        auto group = std::make_shared<GltfMeshGroup>();
        group->name = gltfMesh.name;
        for (auto &gltfPrimitive : gltfMesh.primitives)
        {
            group->primitives.push_back({});
            auto &prim = group->primitives.back();
            prim.LoadVertices(m_gltf, m_bin, gltfMesh, gltfPrimitive);
            prim.mesh->indices = LoadIndices(m_gltf, m_bin, gltfPrimitive);
            if (!prim.mesh->indices)
            {
                throw "not indices";
            }
            // if (gltfPrimitive.material.has_value())
            {
                auto material = m_model->materials[gltfPrimitive.material.value()];
                prim.mesh->submeshes.push_back({
                    .drawCount = prim.mesh->indices->Count(),
                    .material = material,
                });
            }
        }
        return group;
    }

    SceneMeshSkinPtr
    CreateSkin(const gltfformat::Skin &gltfSkin,
               const std::shared_ptr<GltfMeshGroup> &meshGroup)
    {
        auto skin = std::make_shared<SceneMeshSkin>();
        for (auto j : gltfSkin.joints)
        {
            skin->joints.push_back(m_model->nodes[j]);
        }
        if (gltfSkin.inverseBindMatrices.has_value())
        {
            auto accessor = m_gltf.accessors[gltfSkin.inverseBindMatrices.value()];
            auto [p, size] = m_bin.get_bytes(accessor);
            skin->inverseBindMatrices.assign((std::array<float, 16> *)p, (std::array<float, 16> *)(p + size));
        }

        for (auto &primitive : meshGroup->primitives)
        {
            std::copy(primitive.skining.begin(), primitive.skining.end(), std::back_inserter(skin->vertexSkiningArray));
        }

        if (gltfSkin.skeleton.has_value())
        {
            skin->root = m_model->nodes[gltfSkin.skeleton.value()]->Parent();
        }

        return skin;
    }

    void BuildHierarchy()
    {
        // build node hierarchy
        for (auto &meshGroup : m_groups)
        {
            // auto mesh = framedata::FrameMesh::Create(group->name);
            framedata::FrameMeshPtr mesh;
            auto sum = 0;
            for (auto &primitive : meshGroup->primitives)
            {
                sum += primitive.mesh->vertices->Count();
                if (!mesh)
                {
                    mesh = primitive.mesh;
                }
                else
                {
                    mesh->AddSubmesh(primitive.mesh);
                }
            }
            assert(mesh->vertices->Count() == sum);
            m_model->meshes.push_back(mesh);
        }

        for (int i = 0; i < m_model->nodes.size(); ++i)
        {
            auto &gltfNode = m_gltf.nodes[i];
            auto node = m_model->nodes[i];
            if (gltfNode.mesh.has_value())
            {
                auto mesh = m_model->meshes[gltfNode.mesh.value()];
                auto group = m_groups[gltfNode.mesh.value()];

                if (gltfNode.skin.has_value())
                {
                    auto &gltfSkin = m_gltf.skins[gltfNode.skin.value()];
                    node->skin = CreateSkin(gltfSkin, group);
                    m_model->skins.push_back(node->skin);
                    mesh->vertices->isDynamic = true;

                    if (node->skin->vertexSkiningArray.size() != mesh->vertices->Count())
                    {
                        throw;
                    }

                    LOGD << mesh->vertices->Count() << "vertices";
                }

                node->Mesh(mesh);
            }
            for (auto child : gltfNode.children)
            {
                auto childNode = m_model->nodes[child];
                node->AddChild(childNode);
            }
        }
    }

    SceneNodePtr CreateRoot()
    {
        auto root = SceneNode::Create("gltf");
        for (auto node : m_model->nodes)
        {
            if (!node->Parent())
            {
                root->AddChild(node);
            }
        }
        root->UpdateWorld();

        return root;
    }

    SceneModelPtr Load()
    {
        LoadTextures();
        LoadMaterials();
        LoadNodes();
        LoadMeshes();
        BuildHierarchy();
        m_model->root = CreateRoot();
        return m_model;
    }
};

SceneModelPtr SceneModel::LoadFromPath(const std::filesystem::path &path)
{
    auto bytes = read_allbytes(path);
    if (bytes.empty())
    {
        // LOGW << "fail to read bytes: " << path.filename().c_str();
        return nullptr;
    }

    auto model = LoadGlbBytes(bytes.data(), (int)bytes.size());
    if (!model)
    {
        // LOGW << "fail to load: " << path.filename().c_str();
        return nullptr;
    }

    // LOGI << "load: " << path.filename().c_str();
    model->name = (const char *)path.filename().u8string().c_str();
    model->root->Name(model->name);

    return model;
}

SceneModelPtr SceneModel::LoadGlbBytes(const uint8_t *bytes, int byteLength)
{
    gltfformat::glb glb;
    if (!glb.load(bytes, byteLength))
    {
        return nullptr;
    }

    auto gltf = ::ParseGltf(glb.json.p, glb.json.size);

    GltfLoader loader(gltf, glb.bin.p, glb.bin.size);

    return loader.Load();
}

} // namespace hierarchy
