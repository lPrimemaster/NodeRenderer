#pragma once
#include "node.h"
#include "../../math/vector.h"
#include "../../../glm/glm/glm.hpp"
#include "../../../muparser/include/muParser.h"
#include <sstream>

struct ListJoinNode final : public PropertyNode
{
    inline ListJoinNode() : PropertyNode(EmptyType())
    {
        static int inc = 0;
        setInputsOrdered(
            {
                "List A",
                "List B"
            }
        );
        // TODO: This feature needs to be tested first
        // setOutputsOrdered(
        //     {
        //         "List",
        //         "Size"
        //     }
        // );
        _output_count = 1;

        name = "List Join Node #" + std::to_string(inc++);
    }
    
    ~ListJoinNode()
    {
        
    }

    
    inline virtual void render() override
    {
        data.resetDataUpdate();

        // TODO: Create a template indirection for multiple list types
        // Making code maintenance easier in the future
        disconnectInputIfNotOfType<
            std::vector<float>, 
            std::vector<int>, 
            std::vector<unsigned int>,
            std::vector<Vector2>,
            std::vector<Vector3>,
            std::vector<Vector4>
        >("List A");

        disconnectInputIfNotOfType<
            std::vector<float>, 
            std::vector<int>, 
            std::vector<unsigned int>,
            std::vector<Vector2>,
            std::vector<Vector3>,
            std::vector<Vector4>
        >("List B");

        auto listAit = inputs_named.find("List A");
        auto listBit = inputs_named.find("List B");

        if(listAit != inputs_named.end())
        {
            auto& listAData = listAit->second->data;

                 if(joinSimilarListTypesIfOfType<std::vector<float>>       (listAData, listBit));
            else if(joinSimilarListTypesIfOfType<std::vector<int>>         (listAData, listBit));
            else if(joinSimilarListTypesIfOfType<std::vector<unsigned int>>(listAData, listBit));
            else if(joinSimilarListTypesIfOfType<std::vector<Vector2>>     (listAData, listBit));
            else if(joinSimilarListTypesIfOfType<std::vector<Vector3>>     (listAData, listBit));
            else if(joinSimilarListTypesIfOfType<std::vector<Vector4>>     (listAData, listBit)) {  }
        }
        else if(listBit != inputs_named.end())
        {
            auto& listBData = listBit->second->data;
            if(listBData.dataChanged() || inputs.size() != linput_size)
            {
                     if(listBData.isOfType<std::vector<float>>())        data.setValue(listBData.getValue<std::vector<float>>());
                else if(listBData.isOfType<std::vector<int>>())          data.setValue(listBData.getValue<std::vector<int>>());
                else if(listBData.isOfType<std::vector<unsigned int>>()) data.setValue(listBData.getValue<std::vector<unsigned int>>());
                else if(listBData.isOfType<std::vector<Vector2>>())      data.setValue(listBData.getValue<std::vector<Vector2>>());
                else if(listBData.isOfType<std::vector<Vector3>>())      data.setValue(listBData.getValue<std::vector<Vector3>>());
                else if(listBData.isOfType<std::vector<Vector4>>())      data.setValue(listBData.getValue<std::vector<Vector4>>());
            }
        }
        else if(inputs.size() == 0 && inputs.size() != linput_size)
        {
            data.setValue(EmptyType());
        }

        linput_size = (unsigned int)inputs.size();
    }

private:

    template<typename ListType>
    inline bool joinSimilarListTypesIfOfType(PropertyGenericData& fixed, std::map<std::string, PropertyNode *>::iterator it)
    {
        if(fixed.isOfType<ListType>())
        {
            if(it != inputs_named.end())
            {
                auto& other = it->second->data;
                if(other.isOfType<ListType>())
                {
                    if(fixed.dataChanged() || other.dataChanged() || inputs.size() != linput_size)
                    {
                        ListType destination;
                        ListType valA = fixed.getValue<ListType>();
                        ListType valB = other.getValue<ListType>();
                        destination.reserve(valA.size() + valB.size());
                        destination.insert(destination.end(), valA.begin(), valA.end());
                        destination.insert(destination.end(), valB.begin(), valB.end());
                        data.setValue(destination);
                    }
                }
                else
                {
                    L_WARNING("Error trying to join lists with different types:");
                    L_WARNING("Type : %s", fixed.type.name());
                    L_WARNING("Type : %s", other.type.name());
                }
            }
            else if(fixed.dataChanged() || inputs.size() != linput_size)
            {
                ListType valA = fixed.getValue<ListType>();
                data.setValue(valA);
            }
            return true;
        }
        return false;
    }

    unsigned int linput_size = 0;
    int currenttypeid = 0;
    int lasttypeid = 0;
};
