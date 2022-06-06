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
        UE_DS_SUCCESS = 1,
        UE_DS_ERROR_EXE_NAME_NULL = 2,
        UE_DS_ERROR_EXE_NOT_FOUND = 3,
        UE_DS_ERROR_IO_OPERATION = 4,
        UE_DS_ERROR_INIT_GNAMES = 5,
        UE_DS_ERROR_INIT_NAMEPOOL = 6,
        UE_DS_ERROR_INIT_GUOBJECTARRAY = 7,
        UE_DS_ERROR_INIT_OFFSETS = 8,
        UE_DS_ERROR_EMPTY_PACKAGES = 9
    };

    struct DumpArgs
    {
        std::string dump_dir;
        std::string dump_headers_dir;

        bool dump_objects;
        bool dump_full;
        bool dump_headers;

        bool gen_functions_script;
    };

    DumpStatus Dump(DumpArgs *args, IGameProfile *profile);

    std::string DumpStatusToStr(DumpStatus ds);

}