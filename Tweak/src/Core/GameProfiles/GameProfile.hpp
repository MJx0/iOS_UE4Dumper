#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <functional>

#include "../Offsets.hpp"
#include "../../Utils/memory.hpp"

#include <KittyInclude.hpp>

class IGameProfile
{
public:
    virtual ~IGameProfile() = default;

    virtual MemoryFileInfo GetExecutableInfo() const = 0;

    virtual std::vector<std::string> GetAppIDs() const = 0;

    virtual bool IsUsingFNamePool() const = 0;
    
    virtual bool isUsingOutlineNumberName() const = 0;

    virtual uintptr_t GetGUObjectArrayPtr() const = 0;

    // GNames / NamePoolData
    virtual uintptr_t GetNamesPtr() const = 0;

    virtual UE_Offsets *GetOffsets() const = 0;
    
    
    // std::function<std::string(int)>
    // custom method in case game has different implementation to get names
    inline virtual NameByIndex_t CustomNameByIndex() const
    {
        return nullptr;
    }
    
    // std::function<uint8_t*(int)>;
    // custom method in case game has different implementation to get objects
    inline virtual ObjectByIndex_t CustomObjectByIndex() const
    {
        return nullptr;
    }
};
