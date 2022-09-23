#pragma once
#include <unordered_map>
#include <chrono>

namespace Utils
{
    inline bool TriggerConditionAfterXSeconds(const bool* condition, float fseconds)
    {
        // printf("%x\n", condition);
        static std::unordered_map<const bool*, std::chrono::steady_clock::time_point> retvalmap;
        auto it = retvalmap.find(condition);
        if(it != retvalmap.end())
        {
            // L_DEBUG("Cond %u", *condition);
            if(
                *condition && (
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - it->second
                    ).count() > (unsigned int)(fseconds * 1000)
                )
            ) { return true; }

            if(!(*condition))
            {
                L_DEBUG("Erase");
                retvalmap.erase(condition);
            }

            return false;
        }

        if(*condition)
        {
            // Time it to flip retvalmap
            L_DEBUG("Create");
            retvalmap[condition] = std::chrono::steady_clock::now();
            return false;
        }

        return false;
    }
}