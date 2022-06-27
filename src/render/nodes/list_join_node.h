#pragma once
#include "node.h"
#include "../../math/vector.h"
#include "../../../glm/glm/glm.hpp"
#include "../../../muparser/include/muParser.h"
#include <sstream>

struct ListJoinNode final : public PropertyNode
{
    inline ListJoinNode() : PropertyNode(Type::LISTJOIN, 2, { "list A", "list B" }, 2, { "list", "size" })
    {
        static int inc = 0;
        name = "List Join Node #" + std::to_string(inc++);
    }
    
    ~ListJoinNode()
    {
        
    }

    
    inline virtual void render() override
    {
        resetOutputsDataUpdate();
        
        disconnectInputIfNotOfType<
            std::vector<float>, 
            std::vector<int>, 
            std::vector<unsigned int>,
            std::vector<Vector2>,
            std::vector<Vector3>,
            std::vector<Vector4>
        >("list A");

        disconnectInputIfNotOfType<
            std::vector<float>, 
            std::vector<int>, 
            std::vector<unsigned int>,
            std::vector<Vector2>,
            std::vector<Vector3>,
            std::vector<Vector4>
        >("list B");

        auto listAit = inputs_named.find("list A");
        auto listBit = inputs_named.find("list B");

        if(listAit != inputs_named.end())
        {
            auto listAData = listAit->second;

                 if(joinSimilarListTypesIfOfType<std::vector<float>>       (listAData, listBit));
            else if(joinSimilarListTypesIfOfType<std::vector<int>>         (listAData, listBit));
            else if(joinSimilarListTypesIfOfType<std::vector<unsigned int>>(listAData, listBit));
            else if(joinSimilarListTypesIfOfType<std::vector<Vector2>>     (listAData, listBit));
            else if(joinSimilarListTypesIfOfType<std::vector<Vector3>>     (listAData, listBit));
            else if(joinSimilarListTypesIfOfType<std::vector<Vector4>>     (listAData, listBit)) {  }
        }
        else if(listBit != inputs_named.end())
        {
            auto listBData = listBit->second;
            if(listBData->dataChanged() || inputs.size() != linput_size)
            {
                if(listBData->isOfType<std::vector<float>>())        
                { setNamedOutput("list", listBData->getValue<std::vector<float>>());        setNamedOutput("size", (unsigned int)listBData->getValue<std::vector<float>>().size());        }
                else if(listBData->isOfType<std::vector<int>>())          
                { setNamedOutput("list", listBData->getValue<std::vector<int>>());          setNamedOutput("size", (unsigned int)listBData->getValue<std::vector<int>>().size());          }
                else if(listBData->isOfType<std::vector<unsigned int>>()) 
                { setNamedOutput("list", listBData->getValue<std::vector<unsigned int>>()); setNamedOutput("size", (unsigned int)listBData->getValue<std::vector<unsigned int>>().size()); }
                else if(listBData->isOfType<std::vector<Vector2>>())      
                { setNamedOutput("list", listBData->getValue<std::vector<Vector2>>());      setNamedOutput("size", (unsigned int)listBData->getValue<std::vector<Vector2>>().size());      }
                else if(listBData->isOfType<std::vector<Vector3>>())      
                { setNamedOutput("list", listBData->getValue<std::vector<Vector3>>());      setNamedOutput("size", (unsigned int)listBData->getValue<std::vector<Vector3>>().size());      }
                else if(listBData->isOfType<std::vector<Vector4>>())      
                { setNamedOutput("list", listBData->getValue<std::vector<Vector4>>());      setNamedOutput("size", (unsigned int)listBData->getValue<std::vector<Vector4>>().size());      }
            }
        }
        else if(inputs.size() == 0 && inputs.size() != linput_size)
        {
            setNamedOutput("list", EmptyType());
            setNamedOutput("size", 0U);
        }

        linput_size = (unsigned int)inputs.size();
    }

private:

    template<typename ListType>
    inline bool joinSimilarListTypesIfOfType(PropertyGenericData* fixed, std::map<std::string, PropertyGenericData*>::iterator it)
    {
        if(fixed->isOfType<ListType>())
        {
            if(it != inputs_named.end())
            {
                auto other = it->second;
                if(other->isOfType<ListType>())
                {
                    if(fixed->dataChanged() || other->dataChanged() || inputs.size() != linput_size)
                    {
                        ListType destination;
                        ListType valA = fixed->getValue<ListType>();
                        ListType valB = other->getValue<ListType>();
                        destination.reserve(valA.size() + valB.size());
                        destination.insert(destination.end(), valA.begin(), valA.end());
                        destination.insert(destination.end(), valB.begin(), valB.end());
                        setNamedOutput("list", destination);
                        setNamedOutput("size", (unsigned int)destination.size());
                    }
                }
                else
                {
                    L_WARNING("Error trying to join lists with different types:");
                    L_WARNING("Type : %s", fixed->type.name());
                    L_WARNING("Type : %s", other->type.name());
                }
            }
            else if(fixed->dataChanged() || inputs.size() != linput_size)
            {
                ListType valA = fixed->getValue<ListType>();
                setNamedOutput("list", valA);
                setNamedOutput("size", (unsigned int)valA.size());
            }
            return true;
        }
        return false;
    }

    unsigned int linput_size = 0;
    int currenttypeid = 0;
    int lasttypeid = 0;
};
