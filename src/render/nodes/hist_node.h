#pragma once
#include "node.h"
#include "../../../implot/implot.h"
#include "../../util/imgui_ext.inl"

template<typename T, typename U>
struct AnyVec2
{
    constexpr AnyVec2() {  }
    constexpr AnyVec2(const T& x, const U& y) : x(x), y(y) {  }
    T x;
    U y;
};

template<typename T, typename U, size_t MAX>
struct ScrollingBuffer
{
    using TVec = AnyVec2<T, U>;
    ScrollingBuffer(size_t curr_max_size = 1000ULL)
    {
        this->curr_max_size = curr_max_size;
        offset = 0ULL;
        size = 0ULL;
    }

    inline void addPoint(const T& x, const U& y)
    {
        curr_x = x;
        if(size < curr_max_size)
        {
            least_x = data.at(0).x;
            data.at(size) = TVec(x, y);
            size++;
        }
        else
        {
            size_t new_offset = (offset + 1) % curr_max_size;
            least_x = data.at(new_offset).x;
            data.at(offset) = TVec(x, y);
            offset = new_offset;
        }
    }

    inline U getYMax()
    {
        auto max_e = std::max_element(data.begin(), data.begin() + size, [](const TVec& a, const TVec& b) -> bool {
            return a.y < b.y;
        });
        return (*max_e).y;
    }

    inline U getYMin()
    {
        auto min_e = std::min_element(data.begin(), data.begin() + size, [](const TVec& a, const TVec& b) -> bool {
            return a.y < b.y;
        });
        return (*min_e).y;
    }

    inline U getXMax()
    {
        auto max_e = std::max_element(data.begin(), data.begin() + size, [](const TVec& a, const TVec& b) -> bool {
            return a.x < b.x;
        });
        return (*max_e).x;
    }

    inline U getXMin()
    {
        auto min_e = std::min_element(data.begin(), data.begin() + size, [](const TVec& a, const TVec& b) -> bool {
            return a.x < b.x;
        });
        return (*min_e).x;
    }


    inline void erase()
    {
        size = 0;
        offset = 0;
    }

    size_t size;
    size_t curr_max_size;
    size_t offset;
    std::array<TVec, MAX> data;
    T curr_x;
    T least_x;
};

struct GraphNode final : public PropertyNode
{
    inline GraphNode() : PropertyNode(Type::GRAPH, 2, { "x", "y" }, 0)
    {
        static int inc = 0;
        name = "Graph Node #" + std::to_string(inc++);

        inputs_description["x"] = "x value to graphically display.";
        inputs_description["y"] = "y value to graphically display.";
    }
    
    ~GraphNode() {  }

    // TODO: Rename this to graph node as well
    inline virtual void render() override
    {
        const char* const modes[] = {"Scrolling x", "Fixed x"};

        ImGui::Combo("Mode", &graph_mode, modes, 2);

        if(assign_value && linename)
        {
            if(ImPlot::BeginPlot("##GraphWindow", ImVec2(0, 0), ImPlotFlags_NoTitle | ImPlotFlags_NoFrame | ImPlotFlags_NoChild))
            {
                float x_max;
                float x_min;
                
                if(graph_mode == 0) // Scrolling x mode
                {
                    x_max = scrolling_buffer.curr_x;
                    x_min = scrolling_buffer.least_x;
                }
                else if(graph_mode == 1) // Fixed x mode
                {
                    x_max = scrolling_buffer.getXMax();
                    x_min = scrolling_buffer.getXMin();
                }

                float min_y = scrolling_buffer.getYMin();
                float max_y = scrolling_buffer.getYMax();

                // L_TRACE("min_y: %f", min_y);
                // L_TRACE("max_y: %f", max_y);

                bool paused = false;
                ImPlot::SetupAxisLimits(ImAxis_X1, x_min, x_max, paused ? ImGuiCond_Once : ImGuiCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, scrolling_buffer.getYMin(), scrolling_buffer.getYMax(), ImGuiCond_Always);
                ImPlot::PlotLine(
                    linename->c_str(),
                    &scrolling_buffer.data[0].x,
                    &scrolling_buffer.data[0].y,
                    scrolling_buffer.size,
                    0,
                    scrolling_buffer.offset,
                    2 * sizeof(float)
                );
                ImPlot::EndPlot();
            }
        }
        else
        {
            ImGuiExt::SpinnerText();
            ImGui::SameLine();
            ImGui::Text("Waiting for x/y data...");
        }
    }

    virtual void update() override
    {
        float x;
        float y;
        assign_value = false;

        disconnectInputIfNotOfType<float, int, unsigned int>("x");
        disconnectInputIfNotOfType<float, int, unsigned int>("y");

        auto in_x = inputs_named.find("x");
        if(in_x != inputs_named.end())
        {
            PropertyGenericData::TypeDataBuffer buffer = in_x->second->getValueDynamic();

            switch (buffer.vtype)
            {
            case PropertyGenericData::ValidType::FLOAT:
                x = *(float*)buffer.data;
                assign_value = true;
                break;
            case PropertyGenericData::ValidType::INT:
                x = (float)*(int*)buffer.data;
                assign_value = true;
                break;
            case PropertyGenericData::ValidType::UINT:
                x = (float)*(unsigned int*)buffer.data;
                assign_value = true;
                break;
            
            default:
                L_ERROR("GraphNode only suppports inputs of type FLOAT, INT and UNSIGNED INT.");
                break;
            }
        }

        auto in_y = inputs_named.find("y");
        if(in_y != inputs_named.end())
        {
            PropertyGenericData::TypeDataBuffer buffer = in_y->second->getValueDynamic();

            switch (buffer.vtype)
            {
            case PropertyGenericData::ValidType::FLOAT:
                y = *(float*)buffer.data;
                assign_value &= true;
                break;
            case PropertyGenericData::ValidType::INT:
                y = (float)*(int*)buffer.data;
                assign_value &= true;
                break;
            case PropertyGenericData::ValidType::UINT:
                y = (float)*(unsigned int*)buffer.data;
                assign_value &= true;
                break;
            
            default:
                assign_value = false;
                L_ERROR("GraphNode only suppports inputs of type FLOAT, INT and UNSIGNED INT.");
                break;
            }
        }
        else
        {
            assign_value = false;
        }

        if(assign_value)
        {
            scrolling_buffer.addPoint(x, y);
        }
    }

    inline virtual void onConnection(const std::string& inputName) override
    {
        if(inputName == "y")
        {
            PropertyGenericData* ygdata = inputs_named.find(inputName)->second;
            linename = &ygdata->_data_holder_instance->name;
        }
    }

    inline virtual ByteBuffer serialize() const override
    {
        ByteBuffer buffer = PropertyNode::serialize();

        buffer.add(graph_mode);

        return buffer;
    }

    inline virtual void deserialize(ByteBuffer& buffer) override
    {
        PropertyNode::deserialize(buffer);

        buffer.get(&graph_mode);
    }

private:
    int graph_mode = 1;
    ScrollingBuffer<float, float, 4096> scrolling_buffer;
    std::string* linename = nullptr;
    bool assign_value = false;
};
