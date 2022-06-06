#pragma once

#include <types.h>

#include <cstdint>
#include <string>

#include "../Offsets.hpp"
#include "../memory.hpp"

#include <KittyMemory/KittyMemory.hpp>
#include <KittyMemory/KittyScanner.hpp>
#include <KittyMemory/KittyArm64.hpp>

using KittyMemory::memory_file_info;

class IGameProfile
{
public:
    virtual ~IGameProfile() = default;

    virtual memory_file_info GetExecutableInfo()
    {
        return {};
    }

    virtual bool IsUsingFNamePool()
    {
        return false;
    }

    virtual uintptr_t GetGUObjectArrayPtr()
    {
        return 0;
    }

    virtual uintptr_t GetFNamePoolDataPtr()
    {
        return 0;
    }

    virtual uintptr_t GetGNamesPtr()
    {
        return 0;
    }

    virtual Offsets *GetOffsets()
    {
        return NULL;
    }
};