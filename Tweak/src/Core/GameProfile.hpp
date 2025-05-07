#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <functional>
#include <unordered_map>

#include "Offsets.hpp"
#include "../Utils/memory.hpp"

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
    
    virtual std::string GetNameByID(int32_t id) const;
    
protected:
    virtual uint8_t *GetNameEntry_Internal(int32_t id) const;
    virtual std::string GetNameEntryString_Internal(uint8_t *entry) const;
    virtual std::string GetNameByID_Internal(int32_t id) const;
};
