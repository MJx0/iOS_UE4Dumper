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

    virtual memory_file_info GetExecutableInfo() = 0;

    virtual bool IsUsingFNamePool() = 0;

    virtual uintptr_t GetGUObjectArrayPtr() = 0;

    // GNames / FNamePoolData
    virtual uintptr_t GetNamesPtr() = 0;

    virtual Offsets *GetOffsets() = 0; 
};