#pragma once

#include "GameProfile.hpp"

// Distyle
// UE 4.23 / UE 4.24 ??

class DistyleProfile : public IGameProfile
{
public:
    DistyleProfile() = default;

    std::string GetAppID() const override
    {
        return "com.lilithgames.xgame.ios.global";
    }

    MemoryFileInfo GetExecutableInfo() const override
    {
        return KittyMemory::getMemoryFileInfo("XGame");
    }

    bool IsUsingFNamePool() const override
    {
        return true;
    }

    uintptr_t GetGUObjectArrayPtr() const override
    {
        const mach_header *hdr = GetExecutableInfo().header;

        // FUObjectArray::FUObjectArray();
        const char *hex = "E1 23 00 91 00 00 00 94 E0 23 00 91 00 00 00 94 08 7D 80 52";
        const char *mask = "xxxx???xxxxx???xxxxx";
        const int step = 0x18;

        uintptr_t insn_address = KittyScanner::findHexFirst(hdr, "__TEXT", hex, mask);
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

        // FNameEntry const* FName::GetEntry(FNameEntryId id);
        const char *hex = "F6 57 BD A9 F4 4F 01 A9 FD 7B 02 A9 FD 83 00 91 F3 03 00 AA 00 00 00 00 A8 02 00 39";
        const char *mask = "xxxxxxxxxxxxxxxxxxxx????xx?x";
        const int step = 0x1C;

        uintptr_t insn_address = KittyScanner::findHexFirst(hdr, "__TEXT", hex, mask);
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

    Offsets *GetOffsets() const override
    {
        struct
        {
            uint16 Stride = 2;             // alignof(FNameEntry)
            uint16 FNamePoolBlocks = 0xD0; // usually ios at 0xD0 and android at 0x40
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
            { // not needed in UE4.23+
                uint16 Name = 0;
            } FNameEntry;
            struct
            {
                uint16 Info = 0;       // Offset to Memory filled with info about type and size of string
                uint16 WideBit = 0;    // Offset to bit which shows if string uses wide characters
                uint16 LenBit = 6;     // Offset to bit which has lenght of string
                uint16 HeaderSize = 2; // Size of FNameEntry header (offset where a string begins)
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
                uint16 OuterPrivate = 0x20;
            } UObject;
            struct
            {
                uint16 Next = 0x28; // sizeof(UObject)
            } UField;
            struct
            {
                uint16 SuperStruct = 0x40; // sizeof(UField) + 2 pointers?
                uint16 Children = 0x48;    // UField*
                uint16 ChildrenProps = 0;  // not needed in versions older than UE4.25
                uint16 PropertiesSize = 0x50;
            } UStruct;
            struct
            {
                uint16 Names = 0x40; // usually at sizeof(UField) + sizeof(FString)
            } UEnum;
            struct
            {
                uint16 EFunctionFlags = 0x98; // sizeof(UStruct)
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