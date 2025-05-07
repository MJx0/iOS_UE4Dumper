#include "GameProfile.hpp"

#include <utfcpp/unchecked.h>

uint8_t *IGameProfile::GetNameEntry_Internal(int32_t id) const
{
    if (id < 0) return nullptr;
    
    uintptr_t namesPtr = GetNamesPtr();
    if (namesPtr == 0) return nullptr;
    
    if (!IsUsingFNamePool())
    {
        const int32_t ElementsPerChunk = 16384;
        const int32_t ChunkIndex = id / ElementsPerChunk;
        const int32_t WithinChunkIndex = id % ElementsPerChunk;
        
        // FNameEntry**
        uint8_t *FNameEntryArray = vm_rpm_ptr<uint8_t *>((void*)(namesPtr + ChunkIndex * sizeof(uintptr_t)));
        if (!FNameEntryArray) return nullptr;
        
        // FNameEntry*
        return vm_rpm_ptr<uint8_t *>(FNameEntryArray + WithinChunkIndex * sizeof(uintptr_t));
    }
    
    uint32_t block_offset = ((id >> 16) * sizeof(void*));
    uint32_t chunck_offset = ((id & 0xffff) * GetOffsets()->Stride);
    
    uint8_t *chunck = vm_rpm_ptr<uint8_t *>((void *)(namesPtr + GetOffsets()->FNamePoolBlocks + block_offset));
    if (!chunck) return nullptr;
    
    return (chunck + chunck_offset);
}

std::string IGameProfile::GetNameEntryString_Internal(uint8_t *entry) const
{
    if (!entry) return "";
    
    UE_Offsets *offsets = GetOffsets();
    
    uint8_t *pStr = nullptr;
    bool isWide = false;
    size_t strLen = 0;
    int strNumber = 0;
    
    if (!IsUsingFNamePool())
    {
        int32_t name_index = 0;
        if (!vm_rpm_ptr(entry, &name_index, sizeof(int32_t))) return "";
        
        pStr = entry + offsets->FNameEntry.Name;
        isWide = (name_index & 1);
        strLen = offsets->FNameMaxSize;
    }
    else
    {
        int16_t header = 0;
        if (!vm_rpm_ptr(entry + offsets->FNameEntry23.Header, &header, sizeof(int16_t)))
            return "";
        
        if (isUsingOutlineNumberName() && offsets->FNameEntry23.GetLength(header) == 0)
        {
            const int32_t stringOff = offsets->FNameEntry23.Header + sizeof(int16_t);
            const int32_t entryIdOff = stringOff + ((stringOff == 6) * 2);
            const int32_t nextEntryId = vm_rpm_ptr<int32_t>(entry + entryIdOff);
            if (nextEntryId <= 0) return "";
            
            strNumber = vm_rpm_ptr<int32_t>(entry + entryIdOff + sizeof(int32_t));
            entry = GetNameEntry_Internal(nextEntryId);
            if (!vm_rpm_ptr(entry + offsets->FNameEntry23.Header, &header, sizeof(int16_t)))
                return "";
        }
        
        strLen = std::min<size_t>(offsets->FNameEntry23.GetLength(header), offsets->FNameMaxSize);
        if (strLen <= 0) return "";
        
        isWide = offsets->FNameEntry23.GetIsWide(header);
        pStr = entry + offsets->FNameEntry23.Header + sizeof(int16_t);
    }
    
    std::string result = "";
    
    if (isWide)
    {
        std::wstring wstr = vm_rpm_strw(pStr, strLen);
        if (!wstr.empty())
            utf8::unchecked::utf16to8(wstr.begin(), wstr.end(), std::back_inserter(result));
    }
    else
    {
        result = vm_rpm_str(pStr, strLen);
    }
    
    if (strNumber > 0)
        result += '_' + std::to_string(strNumber - 1);
    
    return result;
}

std::string IGameProfile::GetNameByID_Internal(int32_t id) const
{
    return GetNameEntryString_Internal(GetNameEntry_Internal(id));
}

std::string IGameProfile::GetNameByID(int32_t id) const
{
    static std::unordered_map<int32_t, std::string> namesCachedMap;
    if (namesCachedMap.count(id) > 0)
        return namesCachedMap[id];
    
    std::string name = GetNameByID_Internal(id);
    if (!name.empty())
    {
        namesCachedMap[id] = name;
    }
    return name;
}
