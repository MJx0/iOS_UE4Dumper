#pragma once

#include "GameProfile.hpp"

// PUBGM
// UE 4.18

class PUBGMProfile : public IGameProfile
{
public:
    PUBGMProfile() = default;

    std::string GetAppID() const override
    {
        return "com.tencent.ig";
    }

    MemoryFileInfo GetExecutableInfo() const override
    {
        return KittyMemory::getMemoryFileInfo("ShadowTrackerExtra");
    }

    bool IsUsingFNamePool() const override
    {
        return false;
    }

    uintptr_t GetGUObjectArrayPtr() const override
    {
        const mach_header *hdr = GetExecutableInfo().header;

        const char *bytes = "\x80\xB9\x00\x00\x00\x00\x00\x00\x00\x91\x00\x00\x40\xF9\x00\x03\x80\x52";
        const char *mask = "xx???????x??xx?xxx";
        const int step = 2;

        uintptr_t insn_address = KittyScanner::findBytesFirst(hdr, "__TEXT", bytes, mask);
        if (insn_address == 0)
            return 0;

        insn_address += step;

        int64 adrp_pc_rel = 0;
        int32 add_imm12 = 0;

        const int page_size = 4096;
        const uintptr_t page_off = (insn_address & ~(page_size - 1));

        uint32 adrp_insn = vm_rpm_ptr<uint32>((void *)(insn_address));
        uint32 add_insn = vm_rpm_ptr<uint32>((void *)(insn_address + 4));
        if (adrp_insn == 0 || add_insn == 0)
            return 0;

        if (!KittyArm64::decode_adr_imm(adrp_insn, &adrp_pc_rel) || adrp_pc_rel == 0)
            return 0;

        add_imm12 = KittyArm64::decode_addsub_imm(add_insn);
        if (add_imm12 == 0)
            return 0;

        return (page_off + adrp_pc_rel + add_imm12);
    }

    uintptr_t GetNamesPtr() const override
    {
        const mach_header *hdr = GetExecutableInfo().header;

        const char *bytes = "\xE0\x00\x00\x91\x00\x09\x00\x94\x00\x00\x02";
        const char *mask = "x?xx?xxx??x";
        const int step = 8;

        uintptr_t insn_address = KittyScanner::findBytesFirst(hdr, "__TEXT", bytes, mask);
        if (insn_address == 0)
            return 0;
            
        insn_address += step;

        int64 adrp_pc_rel = 0;
        int32 add_imm12 = 0;

        const int page_size = 4096;
        const uintptr_t page_off = (insn_address & ~(page_size - 1));

        uint32 adrp_insn = vm_rpm_ptr<uint32>((void *)(insn_address));
        uint32 add_insn = vm_rpm_ptr<uint32>((void *)(insn_address + 4));
        if (adrp_insn == 0 || add_insn == 0)
            return 0;

        if (!KittyArm64::decode_adr_imm(adrp_insn, &adrp_pc_rel) || adrp_pc_rel == 0)
            return 0;

        add_imm12 = KittyArm64::decode_addsub_imm(add_insn);
        if (add_imm12 == 0)
            return 0;

        // getting randomized GNames ptr
        uintptr_t param_1 = (page_off + adrp_pc_rel + add_imm12);

        int64 var_2;
        int64 var_5[16];

        var_2 = (vm_rpm_ptr<int32>((void *)(param_1)) - 100) / 3u;
        var_5[(uint32)(var_2 - 1)] = vm_rpm_ptr<int64>((void *)(param_1 + 8));

        while (var_2 - 2 >= 0)
        {
            var_5[(uint32)(var_2 - 2)] = vm_rpm_ptr<int64>((void *)(var_5[var_2 - 1]));
            --var_2;
        }

        return vm_rpm_ptr<uintptr_t>((void *)(var_5[0]));
    }

    Offsets *GetOffsets() const override
    {
        struct
        {
            uint16 Stride = 0;          // not needed in versions older than UE4.23
            uint16 FNamePoolBlocks = 0; // not needed in versions older than UE4.23
            uint16 FNameMaxSize = 0xff;
            struct
            {
                uint16 Size = 0x18;
            } FUObjectItem;
            struct
            {
                uint16 Number = 4;
            } FName;
            struct
            {
                uint16 Name = 0xC;
            } FNameEntry;
            struct
            { // not needed in versions older than UE4.23
                uint16 Info = 0;
                uint16 WideBit = 0;
                uint16 LenBit = 0;
                uint16 HeaderSize = 0;
            } FNameEntry23;
            struct
            {
                uint16 ObjObjects = 0x10;
            } FUObjectArray;
            struct
            {
                uint16 NumElements = 0xC;
            } TUObjectArray;
            struct
            {
                uint16 ObjectFlags = 0x8;
                uint16 InternalIndex = 0xC;
                uint16 ClassPrivate = 0x10;
                uint16 NamePrivate = 0x18;
                uint16 OuterPrivate = 0x20;
            } UObject;
            struct
            {
                uint16 Next = 0x28; // sizeof(UObject)
            } UField;
            struct
            {
                uint16 SuperStruct = 0x30; // sizeof(UField)
                uint16 Children = 0x38;    // UField*
                uint16 ChildrenProps = 0;  // not needed in versions older than UE4.25
                uint16 PropertiesSize = 0x40;
            } UStruct;
            struct
            {
                uint16 Names = 0x40; // usually at sizeof(UField) + sizeof(FString)
            } UEnum;
            struct
            {
                uint16 EFunctionFlags = 0x88; // sizeof(UStruct)
                uint16 NumParams = EFunctionFlags + 0x4;
                uint16 ParamSize = NumParams + 0x2;
                uint16 ReturnValueOffset = ParamSize + 0x2;
                uint16 Func = EFunctionFlags + 0x28; // ue3-ue4, always +0x28 from flags location.
            } UFunction;
            struct
            { // not needed in versions older than UE4.25
                uint16 ClassPrivate = 0;
                uint16 Next = 0;
                uint16 NamePrivate = 0;
                uint16 FlagsPrivate = 0;
            } FField;
            struct
            { // not needed in versions older than UE4.25
                uint16 ArrayDim = 0;
                uint16 ElementSize = 0;
                uint16 PropertyFlags = 0;
                uint16 Offset_Internal = 0;
                uint16 Size = 0;
            } FProperty;
            struct
            {
                uint16 ArrayDim = 0x30; // sizeof(UField)
                uint16 ElementSize = 0x34;
                uint16 PropertyFlags = 0x38;
                uint16 Offset_Internal = 0x44;
                uint16 Size = 0x70; // sizeof(FProperty)
            } UProperty;
        } static profile;
        static_assert(sizeof(profile) == sizeof(Offsets));

        return (Offsets *)&profile;
    }
};