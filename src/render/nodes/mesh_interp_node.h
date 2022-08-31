#pragma once
#include "node.h"
#include "mesh_node.h"
#include "../../util/raycaster/raycaster.inl"
#include "../../math/kdtree.inl"
#include "../node_outputs.h"
#include <set>
#include <ciso646>

// This node only works with precooked meshes
// Automatic mesh displacement is not yet working
struct MeshInterpolatorNode final : public PropertyNode
{
    inline MeshInterpolatorNode() : PropertyNode(Type::MESHINTERP, 3, {"t", "mesh A", "mesh B"}, 1, { "Int. mesh" })
    {
        static int inc = 0;
        name = "Mesh Interpolator Node #" + std::to_string(inc++);

        // glGenVertexArrays(1, &_preview_vao);
        // glBindVertexArray(_preview_vao);

        // glGenBuffers(1, &_preview_vbo);
        // glBindBuffer(GL_ARRAY_BUFFER, _preview_vbo);
        // glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

        // GLuint _preview_vao;
        // GLuint _preview_vbo;
        // GLuint _preview_prog;
        // GLuint _preview_fbo;
        // GLuint _preview_tex;
    }
    
    ~MeshInterpolatorNode()
    {

    }

    inline virtual void render() override
    {   
        ImGui::Text("This node linearly interpolates through meshes based on a parameter (t).");
        ImGui::Spacing();

        static const char* const modes[] = {
            "Precooked",
            "Automatic" // 1 (Disabled for now)
        };

        if(ImGui::BeginCombo("Mode", modes[current_mode]))
        {
            for(int i = 0; i < 2; i++)
            {
                bool selected = (i == current_mode);

                if(i == 1) // 1 (Disabled for now)
                {
                    ImGui::BeginDisabled();
                }

                if(ImGui::Selectable(modes[i], selected))
                {
                    current_mode = i;
                }

                if(i == 1) // 1 (Disabled for now)
                {
                    ImGui::EndDisabled();
                }

                if(selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if(size_mismatch_error)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: Mesh vertex size must match.");
        }

        // ImGui::Text("0 <= t <= meshCount");

        if(current_mode == 1) // Automatic
        {
            if(ImGui::InputInt("Raycast samples", &sample_count))
            {
                if(sample_count < 1) sample_count = 1;
            }

            if(ImGui::Button("Generate"))
            {
                needUpdate = true;
            }
        }
    }

    inline virtual void onConnection(const std::string& inputName) override
    {
        if(inputs_named[inputName]->isOfType<MeshNodeData>())
        {
            mesh_list.meshes.push_back(inputs_named[inputName]->getValue<MeshNodeData>());
            mesh_list.changeParamOnly = false;
            outputs[0]->setValue(mesh_list);
            input_mesh_changed = true;
        }
    }

    inline virtual void update() override
    {
        auto data = outputs[0];
        data->resetDataUpdate();

        disconnectInputIfNotOfType<float>("t");
        current_mesh_changed = false;

        for(auto in : inputs_named)
        {
            if(in.second->isOfType<float>()) // Skip the parameter input for update requirement check
            {   
                if(in.second->dataChanged() || !t_param_connected)
                {
                    mesh_list.t = in.second->getValue<float>();

                    // Clamp t -> [0, 1]
                    if(mesh_list.t < 0.0f) mesh_list.t = 0.0f;
                    else if(mesh_list.t > 1.0f) mesh_list.t = 1.0f;
                    mesh_list.changeParamOnly = true;
                    data->setValue(mesh_list);
                }
                t_param_connected = true;
            }
            else
            {
                disconnectInputIfNotOfType<MeshNodeData>(in.first);
                if(in.second->dataChanged())
                {
                    // needUpdate = true;
                    current_mesh_changed = true;
                }
            }
        }
        
        // Here we should always have inputs.size() > 1
        if(current_mode == 1) // Automatic
        {
            updateAutomatic();
        }
        else if(current_mode == 0 && (input_mesh_changed || current_mesh_changed)) // Precooked
        {
            updatePrecooked();
        }
    }

    inline virtual ByteBuffer serialize() const override
    {
        ByteBuffer buffer = PropertyNode::serialize();

        // Just save the current mode
        buffer.add(current_mode);

        return buffer;
    }
    
    inline virtual void deserialize(ByteBuffer& buffer) override
    {
        PropertyNode::deserialize(buffer);

        // Just load the current mode
        buffer.get(&current_mode);
    }

private:
    void updateAutomatic()
    {
        L_ERROR("MeshInterpolatorNode::updateAutomatic() is not supported"); return;

        if(needUpdate)
        {
            needUpdate = false;
            std::vector<MeshNodeData> new_mesh_data;
            new_mesh_data.reserve(inputs.size() - 1);
            for(auto in : inputs)
            {
                if(in.second->isOfType<MeshNodeData>())
                {
                    new_mesh_data.push_back(in.second->getValue<MeshNodeData>());
                }
            }

            int parent_mesh_idx;
            std::vector<Utils::BvhTree*> bvh_list = Utils::ConstructBvhTreesFromMeshes(new_mesh_data, &parent_mesh_idx, true);

            if(parent_mesh_idx < 0 || parent_mesh_idx >= new_mesh_data.size())
            {
                L_ERROR("Something went wrong inside Utils::ConstructBvhTreesFromMeshes()");
            }

            MeshNodeData parent_mesh = new_mesh_data[parent_mesh_idx];
            new_mesh_data.erase(new_mesh_data.begin() + parent_mesh_idx);

            // Now we can raycast, and figure out the new verices
            std::vector<std::vector<Vector3>> new_vertices = Utils::CalculateNewRaycastVerticesIntersection(bvh_list, parent_mesh, sample_count);

            // Utils::DumpObjFileToDiskFromData("dump.obj", new_vertices[0]);

            Utils::DumpPointsToDiskFromData("generated.txt", new_vertices[0]);
            Utils::DumpPointsToDiskFromData("parentMesh.txt", parent_mesh.vertex_data, parent_mesh.data_size, true);
            Utils::DumpPointsToDiskFromData("secondMesh.txt", new_mesh_data[0].vertex_data, new_mesh_data[0].data_size, true);

            // Fit the newly created vertices in a KDTree for comparison with originals for each mesh
            int unresolvedVertices = 0;
            for(size_t i = 0; i < new_mesh_data.size(); i++)
            {
                std::vector<Math::KdNode> nodes;
                nodes.reserve(new_vertices[i].size());

                for(auto v : new_vertices[i]) nodes.emplace_back(v);
                Math::KdTree tree(nodes);

                for(size_t v = 0; v < new_mesh_data[i].data_size; v += 6)
                {
                    Vector3 originalVertex(
                        new_mesh_data[i].vertex_data[v],
                        new_mesh_data[i].vertex_data[v + 1],
                        new_mesh_data[i].vertex_data[v + 2]
                    );

                    // Get the k nearest neighbours
                    constexpr int k = 10;
                    auto nn_3 = tree.kNNSearch(originalVertex, k);

                    bool foundFreeVertex = false;
                    for(size_t n = 0; n < k; n++)
                    {
                        if(!used_nodes_indices.contains(nn_3[n].idx))
                        {
                            used_nodes_indices.insert(nn_3[n].idx);
                            new_vertices[i][nn_3[n].idx] = originalVertex;
                            foundFreeVertex = true;
                            break;
                        }
                    }

                    if(!foundFreeVertex)
                    {
                        // TODO: Expand the search if the nearest neighbour could not be found within the 3 closest candidate points
                        L_ERROR("MeshInterpolatorNode: Could not find a unique generated vertex to replace the original mesh point.");
                        L_TRACE("For Vertex: %u/%u", v / 3, new_mesh_data[i].data_size / 3);
                        L_TRACE("kNN Search: (%.2f %.2f %.2f)", originalVertex.x, originalVertex.y, originalVertex.z);
                        L_TRACE("kNN Candidates Found: (Ordered)");
                        for(size_t n = 0; n < k; n++)
                        {
                            L_TRACE("(%.2f %.2f %.2f)", nn_3[n].point.x, nn_3[n].point.y, nn_3[n].point.z);
                        }
                        L_TRACE("\n\n");
                        unresolvedVertices++;
                    }
                }
                L_TRACE("MeshInterpolatorNode: Unresolved vertices: %d/%u", unresolvedVertices, new_mesh_data[i].data_size / 3);
            }

            // Create the new output mesh with the new generated and fitted points
            MeshNodeData mnd; // TODO

            Utils::FreeBvhTrees(bvh_list);
        }
    }

    void updatePrecooked()
    {
        size_mismatch_error = false;
        size_t lastDataSize = 0;
        for(auto m : mesh_list.meshes)
        {
            if(lastDataSize != 0 && m.data_size != lastDataSize)
            {
                size_mismatch_error = true;
                mesh_list.meshes.clear();
                mesh_list.totalMeshesDataSize = 0;
                disconnectAllInputsIfNotOfType<EmptyType>();
                mesh_list.changeParamOnly = false;
                outputs[0]->setValue(mesh_list);
            }
            else
            {
                lastDataSize = m.data_size;
            }
        }

        if(!size_mismatch_error && current_mesh_changed)
        {
            mesh_list.totalMeshesDataSize = lastDataSize * mesh_list.meshes.size();
            mesh_list.changeParamOnly = false;
            outputs[0]->setValue(mesh_list);
        }
    }

    MeshInterpListData mesh_list;
    int sample_count = 1000;

    std::set<size_t> used_nodes_indices;

    bool t_param_connected = false;
    bool needUpdate = false;

    int current_mode = 0;

    bool size_mismatch_error = false;
    bool input_mesh_changed = false;
    bool current_mesh_changed = false;

    // NOTE: We are inside a gl context, so this should be fine
    // GLuint _preview_vao;
    // GLuint _preview_vbo;
    // GLuint _preview_prog;
    // GLuint _preview_fbo;
    // GLuint _preview_tex;
};
