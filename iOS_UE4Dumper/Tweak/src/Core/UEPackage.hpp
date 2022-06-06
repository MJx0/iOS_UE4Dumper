
#pragma once

#include <cstdio>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>

#include <types.h>

#include "wrappers.hpp"

// Wrapper for 'FILE*' that closes the file handle when it goes out of scope
class File
{
private:
    FILE *file;

public:
    File(const char *path, const char *mode)
    {
        file = fopen(path, mode);
    }
    ~File()
    {
        if (file)
        {
            fclose(file);
        }
    }
    operator bool() const { return file != nullptr; }
    operator FILE *() { return file; }
};

class UE_UPackage
{
private:
    struct Member
    {
        std::string Type;
        std::string Name;
        uint32 Offset = 0;
        uint32 Size = 0;
    };
    struct Function
    {
        std::string Name;
        std::string FullName;
        std::string CppName;
        std::string Params;
        std::string Flags;
        int8 NumParams = 0;
        int16 ParamSize = 0;
        int16 ReturnValueOffset = 0;
        uint64 Func = 0;
    };
    struct Struct
    {
        std::string Name;
        std::string FullName;
        std::string CppName;
        uint32 Inherited = 0;
        uint32 Size = 0;
        std::vector<Member> Members;
        std::vector<Function> Functions;
    };
    struct Enum
    {
        std::string FullName;
        std::string CppName;
        std::vector<std::string> Members;
    };

private:
    std::pair<uint8 *const, std::vector<UE_UObject> > *Package;

public:
    std::vector<Struct> Classes;
    std::vector<Struct> Structures;
    std::vector<Enum> Enums;

private:
    static void GenerateFunction(UE_UFunction fn, Function *out);
    static void GenerateStruct(UE_UStruct object, std::vector<Struct> &arr);
    static void GenerateEnum(UE_UEnum object, std::vector<Enum> &arr);

    static void GenerateBitPadding(std::vector<Member> &members, uint32 offset, uint8 bitOffset, uint8 size);
    static void GeneratePadding(std::vector<Member> &members, uint32 offset, uint32 size);
    static void FillPadding(UE_UStruct object, std::vector<Member> &members, uint32 &offset, uint8 &bitOffset, uint32 end);

    static void SaveStruct(std::vector<Struct> &arr, FILE *file);
    static void SaveEnum(std::vector<Enum> &arr, FILE *file);

public:
    UE_UPackage(std::pair<uint8 *const, std::vector<UE_UObject> > &package) : Package(&package){};
    void Process();
    bool Save(FILE *fulldump_file, const char *dump_dir);
    UE_UObject GetObject() const;
};
