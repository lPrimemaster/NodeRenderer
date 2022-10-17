#pragma once
#include "node.h"
#include "../../../implot/implot.h"
#include "../../util/imgui_ext.inl"
#include <algorithm>

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

    inline void setMaxXSize(size_t size)
    {
        assert(size <= MAX);
        curr_max_size = size;
        offset = 0ULL;
        this->size = 0ULL;
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
        inputs_description["list"] = "list to graphically display.";
    }
    
    ~GraphNode() {  }

    inline virtual void render() override
    {
        const char* const modes[] = {"Scrolling x", "List display"};

        if(ImGui::Combo("Mode", &graph_mode, modes, 2))
        {
            // Mode changed
            // Handle it
            if(graph_mode == 0) // Scrolling x
            {
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("list");
                setInputsOrdered({"x", "y"});
                waiting_message = "x/y";
            }
            else if(graph_mode == 1) // List display
            {
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("x");
                disconnectInputIfNotOfType<PropertyNode::EmptyType>("y");
                setInputsOrdered({"list"});
                waiting_message = "list";
            }
        }

        if(assign_value && linename)
        {
            if(graph_mode == 0) // Scrolling x
            {
                if(ImGui::SliderInt("Scrolling sample size", &scroll_size, 10, 4096))
                {
                    scrolling_buffer.setMaxXSize(scroll_size);
                }
            }
            if(ImPlot::BeginPlot("##GraphWindow", ImVec2(0, 0), ImPlotFlags_NoTitle | ImPlotFlags_NoFrame | ImPlotFlags_NoChild))
            {
                constexpr bool paused = false; // ???
                if(graph_mode == 0) // Scrolling x
                {
                    const float x_max = scrolling_buffer.getXMax();
                    const float x_min = scrolling_buffer.getXMin();

                    // FIXME: This does not work if the limits are zero
                    const float max_y = scrolling_buffer.getYMax() * 1.1f;
                    const float min_y = scrolling_buffer.getYMin() * 1.1f;

                    ImPlot::SetupAxisLimits(ImAxis_X1, x_min, x_max, paused ? ImGuiCond_Once : ImGuiCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, min_y, max_y, ImGuiCond_Always);
                    ImPlot::PlotLine(
                        linename->c_str(),
                        &scrolling_buffer.data[0].x,
                        &scrolling_buffer.data[0].y,
                        scrolling_buffer.size,
                        0,
                        scrolling_buffer.offset,
                        2 * sizeof(float)
                    );
                }
                else if(graph_mode == 1) // List display
                { 
                    auto in_list = inputs_named.find("list");
                    if(in_list != inputs_named.end())
                    {
                        static float y_max;
                        static float y_min;

                        auto list = in_list->second->getValuePtr<std::vector<float>>();
                        if(in_list->second->dataChanged() || first_run)
                        {
                            first_run = false;

                            // Create/Update the x axis for this list
                            list_x_data.resize(list->size());
                            float x_val = 0.0f;
                            for(size_t i = 0; i < list_x_data.size(); i++)
                            {
                                list_x_data[i] = x_val;
                                x_val += 1.0f;
                            }
                            
                            float candidate_y_max = *std::max_element(list->begin(), list->end());
                            if(candidate_y_max > y_max) y_max = candidate_y_max;
                            float candidate_y_min = *std::min_element(list->begin(), list->end());
                            if(candidate_y_min < y_min) y_min = candidate_y_min;
                        }

                        const float x_max = (float)(list_x_data.size() - 1);
                        constexpr float x_min = 0;
                        

                        ImPlot::SetupAxisLimits(ImAxis_X1, x_min, x_max, paused ? ImGuiCond_Once : ImGuiCond_Always);
                        ImPlot::SetupAxisLimits(ImAxis_Y1, y_min, y_max, ImGuiCond_Always);
                        ImPlot::PlotBars(
                            linename->c_str(),
                            list_x_data.data(),
                            list->data(),
                            list_x_data.size(),
                            0.7
                        );
                    }
                    else
                    {
                        first_run = true;
                    }
                }
                ImPlot::EndPlot();
            }
        }
        else
        {
            ImGuiExt::SpinnerText();
            ImGui::SameLine();
            ImGui::Text("Waiting for %s data...", waiting_message.c_str());
        }
    }

    virtual void update() override
    {
        float x;
        float y;
        assign_value = false;

        if(graph_mode == 0) // Scrolling x
        {
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
                    L_ERROR("GraphNode only suppports inputs of type FLOAT, INT and UNSIGNED INT in the current mode.");
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
        else if(graph_mode == 1) // List display
        {
            disconnectInputIfNotOfType<std::vector<float>>("list");
            assign_value = true;
        }
    }

    inline virtual void onConnection(const std::string& inputName) override
    {
        if(inputName == "y" || inputName == "list")
        {
            PropertyGenericData* ygdata = inputs_named.find(inputName)->second;
            linename = &ygdata->_data_holder_instance->name;
        }
    }

    inline virtual ByteBuffer serialize() const override
    {
        ByteBuffer buffer = PropertyNode::serialize();

        buffer.add(graph_mode);
        buffer.add(scroll_size);

        return buffer;
    }

    inline virtual void deserialize(ByteBuffer& buffer) override
    {
        PropertyNode::deserialize(buffer);

        buffer.get(&graph_mode);
        buffer.get(&scroll_size);

        if(graph_mode == 0) // Scrolling x
        {
            setInputsOrdered({"x", "y"});
            waiting_message = "x/y";
        }
        else if(graph_mode == 1) // List display
        {
            setInputsOrdered({"list"});
            waiting_message = "list";
        }
    }

private:
    int graph_mode = 0;
    int scroll_size = 1000;

    ScrollingBuffer<float, float, 4096> scrolling_buffer;
    std::vector<float> list_x_data;
    std::string* linename = nullptr;
    bool assign_value = false;
    bool first_run = true;
    std::string waiting_message;
};
