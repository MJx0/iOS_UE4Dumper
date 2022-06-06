#pragma once

#include "GameProfile.hpp"

// DeadByDaylight
// UE 4.25

class DBDProfile : public IGameProfile
{
public:
    memory_file_info GetExecutableInfo() override
    {
        return KittyMemory::getMemoryFileInfo("DeadByDaylight");
    }

    bool IsUsingFNamePool() override
    {
        return true;
    }

    uintptr_t GetGUObjectArrayPtr() override
    {
        const mach_header_64 *hdr = GetExecutableInfo().header;

        const char *pattern = "\xFF\x9F\x52\x00\x00\x00\x00\x00\x00\x00\x91\x00\x00\x80\x52";
        const char *mask = "xxx???????x??xx";
        const int step = 3;

        uintptr_t insn_address = KittyScanner::find_from_segment64(hdr, "__TEXT", pattern, mask);
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

    uintptr_t GetFNamePoolDataPtr() override
    {
        const mach_header_64 *hdr = GetExecutableInfo().header;

        const char *pattern = "\x37\x00\x00\x02\xD0\x00\x00\x00\x91\x00\x00\xFF\x97\x00\x00\x80\x52";
        const char *mask = "x??xxxx?x??xx?xxx";
        const int step = 1;

        uintptr_t insn_address = KittyScanner::find_from_segment64(hdr, "__TEXT", pattern, mask);
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

    Offsets *GetOffsets() override
    {
        struct
        {
            uint16 Stride = 4;
            uint16 FNamePoolBlocks = 0xD0; // usually ios at 0xD0 and android at 0x40
            uint16 FNameMaxSize = 0xff;
            struct
            {
                uint16 Size = 0x18;
            } FUObjectItem;
            struct
            {
                uint16 Number = 8;
            } FName;
            struct
            { // not needed in UE4.23+
                uint16 Name = 0;
            } FNameEntry;
            struct
            {
                uint16 Info = 4;
                uint16 WideBit = 0;
                uint16 LenBit = 1;
                uint16 HeaderSize = 6;
            } FNameEntry23;
            struct
            {
                uint16 ObjObjects = 0x10;
            } FUObjectArray;
            struct
            {
                uint16 NumElements = 0x14;
            } TUObjectArray;
            struct
            {
                uint16 ObjectFlags = 0x8;
                uint16 InternalIndex = 0xC;
                uint16 ClassPrivate = 0x10;
                uint16 NamePrivate = 0x18;
                uint16 OuterPrivate = 0x28;
            } UObject;
            struct
            {
                uint16 Next = 0x30; // sizeof(UObject)
            } UField;
            struct
            {
                uint16 SuperStruct = 0x48;   // sizeof(UField) + 2 pointers?
                uint16 Children = 0x50;      // UField*
                uint16 ChildrenProps = 0x58; // FField*
                uint16 PropertiesSize = 0x60;
            } UStruct;
            struct
            {
                uint16 Names = 0x48; // usually at sizeof(UField) + 1 pointer
            } UEnum;
            struct
            {
                uint16 EFunctionFlags = 0xB8; // sizeof(UStruct)
                uint16 NumParams = EFunctionFlags + 0x4;
                uint16 ParamSize = NumParams + 0x2;
                uint16 ReturnValueOffset = ParamSize + 0x2;
                uint16 Func = 0xB8 + 0x28; // ue3-ue4, always +0x28 from flags location
            } UFunction;
            struct
            {
                uint16 ClassPrivate = 0x8;
                uint16 Next = 0x20;
                uint16 NamePrivate = 0x28;
                uint16 FlagsPrivate = 0x30;
            } FField;
            struct
            {
                uint16 ArrayDim = 0x38;
                uint16 ElementSize = 0x3C;
                uint16 PropertyFlags = 0x40;
                uint16 Offset_Internal = 0x4C;
                uint16 Size = 0x80; // sizeof(FProperty)
            } FProperty;
            struct
            { // not needed in UE4.23+
                uint16 ArrayDim = 0;
                uint16 ElementSize = 0;
                uint16 PropertyFlags = 0;
                uint16 Offset_Internal = 0;
                uint16 Size = 0;
            } UProperty;
        } static profile;
        static_assert(sizeof(profile) == sizeof(Offsets));

        return (Offsets *)&profile;
    }
};