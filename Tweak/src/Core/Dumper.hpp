#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <string>

#include <types.h>

#include "GameProfiles/GameProfile.hpp"

namespace Dumper
{
    enum DumpStatus : uint8
    {
        UE_DS_NONE = 0,
        UE_DS_SUCCESS,
        UE_DS_ERROR_EXE_NAME_NULL,
        UE_DS_ERROR_EXE_NOT_FOUND,
        UE_DS_ERROR_IO_OPERATION,
        UE_DS_ERROR_INIT_GNAMES,
        UE_DS_ERROR_INIT_NAMEPOOL,
        UE_DS_ERROR_INIT_GUOBJECTARRAY,
        UE_DS_ERROR_INIT_OFFSETS,
        UE_DS_ERROR_EMPTY_PACKAGES
    };

    DumpStatus Dump(const std::string &dir, const std::string &headers_dir, IGameProfile *profile);

    std::string DumpStatusToStr(DumpStatus ds);

}