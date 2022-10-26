#pragma once
#include "node.h"
#include "../node_outputs.h"

struct ShaderNode final : public PropertyNode
{
    inline ShaderNode() : PropertyNode(Type::SHADER, 0, {}, 1, { "out" })
    {
        static int inc = 0;
        name = "Shader Node #" + std::to_string(inc++);

        setOutputNominalTypes<ShaderNodeData>("out", "Returns the generated/input shader code to the raymarcher.");

        // Create a huge buffer for writting code
        code_buffer = new char[1000000];
        code_buffer[0] = '\0';
    }
    
    ~ShaderNode()
    {
        if(code_buffer)
        {
            delete[] code_buffer;
        }
    }

    inline virtual void render() override
    {
        outputs[0]->resetDataUpdate();

        static const char* const modes[] = {
            "GLSL Code",
            "ShaderGraph" // 1 (Disabled for now)
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

        switch (current_mode)
        {
        case 0: //glsl
        {
            code_text_changed |= ImGui::InputTextMultiline("##_glsl_code", 
                code_buffer, 10000, ImVec2(500.0f, 400.0f), 
                ImGuiInputTextFlags_AllowTabInput
            );

            if(!code_text_changed)
            {
                ImGui::BeginDisabled();
            }
            update_data = ImGui::Button("Update");
            if(!code_text_changed)
            {
                ImGui::EndDisabled();
            }

            if(update_data)
            {
                data.generated_code = code_buffer;
                code_text_changed = false;
            }
            

            ImGui::SameLine();
            if(ImGui::Button("How to use..."))
            {
                // TODO
            }
        }
        break;
        
        case 1: //shader graph
        {
            // TODO: Who knows..., maybe I'll implement this.
        }
        break;
        }
    }

    inline virtual void update() override
    {
        outputs[0]->resetDataUpdate();

        if(update_data)
        {
            outputs[0]->setValue(data);
            update_data = false;
        }
    }

    inline virtual ByteBuffer serialize() const override
    {
        ByteBuffer buffer = PropertyNode::serialize();

        buffer.add(current_mode);
        buffer.add(data.generated_code);

        return buffer;
    }

    inline virtual void deserialize(ByteBuffer& buffer) override
    {
        PropertyNode::deserialize(buffer);
        buffer.get(&current_mode);
        buffer.get(&data.generated_code);
        strcpy(code_buffer, data.generated_code.c_str());
    }

private:
    float s = 0.0f;
    int current_mode = 0;
    char* code_buffer = nullptr;
    ShaderNodeData data;
    bool code_text_changed = false;
    bool update_data = false;
};
