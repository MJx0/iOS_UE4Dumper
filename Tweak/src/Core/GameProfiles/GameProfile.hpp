#pragma once

#include <types.h>

#include <cstdint>
#include <string>

#include "../Offsets.hpp"
#include "../memory.hpp"

#include <KittyInclude.hpp>

using KittyMemory::MemoryFileInfo;

class IGameProfile
{
public:
    virtual ~IGameProfile() = default;

    virtual MemoryFileInfo GetExecutableInfo() const = 0;

    virtual std::string GetAppID() const = 0;

    virtual bool IsUsingFNamePool() const = 0;

    virtual uintptr_t GetGUObjectArrayPtr() const = 0;

    // GNames / NamePoolData
    virtual uintptr_t GetNamesPtr() const = 0;

    virtual Offsets *GetOffsets() const = 0;
};