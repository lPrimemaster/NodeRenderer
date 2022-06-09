#pragma once
#include "node.h"
#include "../../util/imgui_ext.inl"
#include "../../python/loader.h"
#include "../../util/audio.h"
#include "../../math/comb.inl"
#include <future>
#include <atomic>
#include <chrono>

struct TestNode final : public PropertyNode
{
    inline TestNode() : PropertyNode(2, { "in 1", "in 2" }, 2, { "out 1", "out 2" })
    {
        static int inc = 0;
        name = "Test Node #" + std::to_string(inc++);
    }
    
    ~TestNode()
    {

    }

    inline virtual void update() override
    {
        resetOutputsDataUpdate();

        auto in1 = inputs_named.find("in 1");
        auto in2 = inputs_named.find("in 2");
        
        setNamedOutput("out 1", in1 != inputs_named.end() ? in1->second->getValue<float>() : 1.0f);
        setNamedOutput("out 2", in2 != inputs_named.end() ? in2->second->getValue<float>() : 2.0f);
    }

    inline virtual void render() override
    {
        
    }
};
