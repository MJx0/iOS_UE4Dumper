#pragma once

#include "GameProfile.hpp"

// Arena Breakout
// UE 4.26

class ArenaBreakoutProfile : public IGameProfile
{
public:
    ArenaBreakoutProfile() = default;

    std::vector<std::string> GetAppIDs() const override
    {
        return { "com.proximabeta.mf.uamo" };
    }

    MemoryFileInfo GetExecutableInfo() const override
    {
        return KittyMemory::getMemoryFileInfo("UAGame");
    }

    bool IsUsingFNamePool() const override
    {
        return true;
    }
    
    bool isUsingOutlineNumberName() const override
    {
        return false;
    }

    uintptr_t GetGUObjectArrayPtr() const override
    {
        std::string ida_pattern = "FF 9F 52 ? ? ? ? ? ? ? 91 ? 03 80 52";
        const int step = 3;
        
        auto text_seg = GetExecutableInfo().getSegment("__TEXT");
        
        uintptr_t insn_address = KittyScanner::findIdaPatternFirst(text_seg.start, text_seg.end, ida_pattern);
        if (insn_address == 0)
            return 0;

        insn_address += step;

        int64_t adrp_pc_rel = 0;
        int32_t add_imm12 = 0;

        const int page_size = 4096;
        const uintptr_t page_off = (insn_address & ~(page_size - 1));

        uint32_t adrp_insn = vm_rpm_ptr<uint32_t>((void *)(insn_address));
        uint32_t add_insn = vm_rpm_ptr<uint32_t>((void *)(insn_address + 4));
        if (adrp_insn == 0 || add_insn == 0)
            return 0;

        if (!KittyArm64::decode_adr_imm(adrp_insn, &adrp_pc_rel) || adrp_pc_rel == 0)
            return 0;

        add_imm12 = KittyArm64::decode_addsub_imm(add_insn);

        return (page_off + adrp_pc_rel + add_imm12);
    }

    uintptr_t GetNamesPtr() const override
    {
        std::string ida_pattern = "91 C8 00 00 37 ? ? ? ? ? ? ? 91 ? ? FF 97 ? 00 80 52";
        const int step = 5;
        
        auto text_seg = GetExecutableInfo().getSegment("__TEXT");
        
        uintptr_t insn_address = KittyScanner::findIdaPatternFirst(text_seg.start, text_seg.end, ida_pattern);
        if (insn_address == 0)
            return 0;

        insn_address += step;

        int64_t adrp_pc_rel = 0;
        int32_t add_imm12 = 0;

        const int page_size = 4096;
        const uintptr_t page_off = (insn_address & ~(page_size - 1));

        uint32_t adrp_insn = vm_rpm_ptr<uint32_t>((void *)(insn_address));
        uint32_t add_insn = vm_rpm_ptr<uint32_t>((void *)(insn_address + 4));
        if (adrp_insn == 0 || add_insn == 0)
            return 0;

        if (!KittyArm64::decode_adr_imm(adrp_insn, &adrp_pc_rel) || adrp_pc_rel == 0)
            return 0;

        add_imm12 = KittyArm64::decode_addsub_imm(add_insn);

        return (page_off + adrp_pc_rel + add_imm12);
    }

    UE_Offsets *GetOffsets() const override
    {
        struct
        {
            uint16_t Stride = 2;             // alignof(FNameEntry)
            uint16_t FNamePoolBlocks = 0xD0; // usually ios at 0xD0 and android at 0x40
            uint16_t FNameMaxSize = 0xff;
            struct
            {
                uint16_t Number = 4;
            } FName;
            struct
            { // not needed in UE4.23+
                uint16_t Name = 0;
            } FNameEntry;
            struct
            {
                uint16_t Header = 0; // Offset to name entry header
                std::function<bool(int16_t)> GetIsWide = [](int16_t header){ return header&1; };
                std::function<size_t(int16_t)> GetLength = [](int16_t header){ return header>>6; };
            } FNameEntry23;
            struct
            {
                uint16_t ObjObjects = 0x10;
            } FUObjectArray;
            struct
            {
                uint16_t Objects = 0;
                uint16_t NumElements = 0x14;
            } TUObjectArray;
            struct
            {
                uint16_t Object = 0;
                uint16_t Size = 0x18;
            } FUObjectItem;
            struct
            {
                uint16_t ObjectFlags = 0x8;
                uint16_t InternalIndex = 0xC;
                uint16_t ClassPrivate = 0x10;
                uint16_t NamePrivate = 0x18;
                uint16_t OuterPrivate = 0x20;
            } UObject;
            struct
            {
                uint16_t Next = 0x28; // sizeof(UObject)
            } UField;
            struct
            {
                uint16_t SuperStruct = 0x40;   // sizeof(UField) + 2 pointers?
                uint16_t Children = 0x48;      // UField*
                uint16_t ChildProperties = 0x50; // FField*
                uint16_t PropertiesSize = 0x58;
            } UStruct;
            struct
            {
                uint16_t Names = 0x40; // usually at sizeof(UField) + sizeof(FString)
            } UEnum;
            struct
            {
                uint16_t EFunctionFlags = 0xB0; // sizeof(UStruct)
                uint16_t NumParams = EFunctionFlags + 0x4;
                uint16_t ParamSize = NumParams + 0x2;
                uint16_t Func = EFunctionFlags + 0x28; // ue3-ue4, always +0x28 from flags location.
            } UFunction;
            struct
            {
                uint16_t ClassPrivate = 0x8;
                uint16_t Next = 0x20;
                uint16_t NamePrivate = 0x28;
                uint16_t FlagsPrivate = 0x30;
            } FField;
            struct
            {
                uint16_t ArrayDim = 0x34; // sizeof(FField)
                uint16_t ElementSize = 0x38;
                uint16_t PropertyFlags = 0x40;
                uint16_t Offset_Internal = 0x4C;
                uint16_t Size = 0x78; // sizeof(FProperty)
            } FProperty;
            struct
            { // not needed in UE4.25+
                uint16_t ArrayDim = 0;
                uint16_t ElementSize = 0;
                uint16_t PropertyFlags = 0;
                uint16_t Offset_Internal = 0;
                uint16_t Size = 0;
            } UProperty;
        } static profile;
        static_assert(sizeof(profile) == sizeof(UE_Offsets));

        return (UE_Offsets *)&profile;
    }
};
