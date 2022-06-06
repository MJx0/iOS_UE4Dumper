#include "wrappers.hpp"

#include <map>
#include <unordered_map>

#include <algorithm>
#include <fmt/core.h>

namespace Profile
{
    uintptr_t BaseAddress = 0;

    bool isUsingFNamePool = false;

    uintptr_t FNamePoolPtr = 0;
    uintptr_t GNamesPtr = 0;
    uintptr_t ObjObjectsPtr = 0;

    TUObjectArray ObjObjects{};

    Offsets offsets{};
}

std::string GetNameByID(uint32 id)
{
    if (Profile::GNamesPtr == 0 && Profile::FNamePoolPtr == 0)
        return "";

    static std::map<uint32, std::string> namesCachedMap;
    if (namesCachedMap.count(id) > 0)
        return namesCachedMap[id];

    std::string name = "";

    if (!Profile::isUsingFNamePool)
    {
        const int32 ElementsPerChunk = 16384;
        const int32 ChunkIndex = id / ElementsPerChunk;
        const int32 WithinChunkIndex = id % ElementsPerChunk;

        // FNameEntry**
        uint8 *FNameEntryArray = vm_rpm_ptr<uint8 *>((void *)(Profile::GNamesPtr + ChunkIndex * sizeof(uintptr_t)));
        if (!FNameEntryArray)
            return name;

        // FNameEntry*
        uint8 *FNameEntryPtr = vm_rpm_ptr<uint8 *>((void *)(FNameEntryArray + WithinChunkIndex * sizeof(uintptr_t)));
        if (!FNameEntryPtr)
            return name;

        // isWide
        int32 name_index = 0;
        if (!vm_rpm_ptr((void *)(FNameEntryPtr), &name_index, sizeof(int32)) || (name_index & 0x1))
        {
            return name;
        }
        name = vm_rpm_str((void *)(FNameEntryPtr + Profile::offsets.FNameEntry.Name), Profile::offsets.FNameMaxSize);
    }
    else
    {
        uint32 block_offset = ((id >> 16) * sizeof(void*));
        uint32 chunck_offset = ((id & 0xffff) * Profile::offsets.Stride);

        uint8 *chunck = vm_rpm_ptr<uint8 *>((void *)(Profile::FNamePoolPtr + Profile::offsets.FNamePoolBlocks + block_offset));
        if (!chunck)
            return name;

        int16 nameHeader = 0;
        if (!vm_rpm_ptr((void *)(chunck + chunck_offset + Profile::offsets.FNameEntry23.Info), &nameHeader, sizeof(int16)))
        {
            return name;
        }

        bool isWide = (nameHeader & 0x1);
        int16 len = (nameHeader >> Profile::offsets.FNameEntry23.LenBit);

        if (len > 0 && len <= Profile::offsets.FNameMaxSize)
        {
            if (!isWide)
            {
                name = vm_rpm_str((void *)(chunck + chunck_offset + Profile::offsets.FNameEntry23.HeaderSize), len);
                // name = std::string((const char *)(namePoolChunck+offset+Profile::offsets.FNameEntry.HeaderSize), len);
            }
        }
    }

    namesCachedMap[id] = name;

    return name;
}

int32 TUObjectArray::GetMaxElements() const
{
    if (Profile::ObjObjectsPtr == 0)
        return 0;
    return vm_rpm_ptr<int32>((void *)(Profile::ObjObjectsPtr + Profile::offsets.TUObjectArray.NumElements - sizeof(int32)));
}

int32 TUObjectArray::GetNumElements() const
{
    if (Profile::ObjObjectsPtr == 0)
        return 0;
    return vm_rpm_ptr<int32>((void *)(Profile::ObjObjectsPtr + Profile::offsets.TUObjectArray.NumElements));
}

uint8 *TUObjectArray::GetObjectPtr(int32 id) const
{
    if (id < 0 || id >= GetNumElements() || !Objects)
        return nullptr;

    if (!Profile::isUsingFNamePool)
    {
        return vm_rpm_ptr<uint8 *>((void *)((uintptr_t)Objects + (id * Profile::offsets.FUObjectItem.Size)));
    }

    const int32 NumElementsPerChunk = 64 * 1024;
    const int32 chunkIndex = id / NumElementsPerChunk;
    const int32 withinChunkIndex = id % NumElementsPerChunk;

    // if (chunkIndex >= NumChunks) return nullptr;

    uint8 *chunk = vm_rpm_ptr<uint8 *>(Objects + chunkIndex);
    if (!chunk)
        return nullptr;

    return vm_rpm_ptr<uint8 *>(chunk + (withinChunkIndex * Profile::offsets.FUObjectItem.Size));
}

UE_UObject TUObjectArray::FindObject(const std::string &name) const
{
    for (int32 i = 0; i < GetNumElements(); i++)
    {
        UE_UObject object = GetObjectPtr(i);
        if (object && object.GetFullName() == name)
        {
            return object;
        }
    }
    return nullptr;
}

void TUObjectArray::ForEachObject(std::function<void(uint8 *)> callback) const
{
    for (int32 i = 0; i < GetNumElements(); i++)
    {
        uint8 *object = GetObjectPtr(i);
        if (!object) continue;

        callback(object);
    }
}

void TUObjectArray::ForEachObjectOfClass(const UE_UClass cmp, std::function<bool(uint8 *)> callback) const
{
    for (int32 i = 0; i < GetNumElements(); i++)
    {
        UE_UObject object = GetObjectPtr(i);
        if (object && object.IsA(cmp) && object.GetName().find("_Default") == std::string::npos)
        {
            if (callback(object)) return;
        }
    }
}

bool TUObjectArray::IsObject(UE_UObject address) const
{
    for (int32 i = 0; i < GetNumElements(); i++)
    {
        UE_UObject object = GetObjectPtr(i);
        if (address == object) return true;
    }
    return false;
}

std::string UE_FName::GetName() const
{
    uint32 index = 0;
    if (!vm_rpm_ptr(object, &index, sizeof(uint32))) return "null";
    
    auto name = GetNameByID(index);
    int32 number = vm_rpm_ptr<int32>(object + Profile::offsets.FName.Number);
    if (number > 0)
    {
        name += '_' + std::to_string(number);
    }
    auto pos = name.rfind('/');
    if (pos != std::string::npos)
    {
        name = name.substr(pos + 1);
    }
    return name;
}

int32 UE_UObject::GetIndex() const
{
    return vm_rpm_ptr<int32>(object + Profile::offsets.UObject.InternalIndex);
};

UE_UClass UE_UObject::GetClass() const
{
    return vm_rpm_ptr<UE_UClass>(object + Profile::offsets.UObject.ClassPrivate);
}

UE_UObject UE_UObject::GetOuter() const
{
    return vm_rpm_ptr<UE_UObject>(object + Profile::offsets.UObject.OuterPrivate);
}

UE_UObject UE_UObject::GetPackageObject() const
{
    UE_UObject package(nullptr);
    for (auto outer = GetOuter(); outer; outer = outer.GetOuter())
    {
        package = outer;
    }
    return package;
}

std::string UE_UObject::GetName() const
{
    auto fname = UE_FName(object + Profile::offsets.UObject.NamePrivate);
    return fname.GetName();
}

std::string UE_UObject::GetFullName() const
{
    std::string temp;
    for (auto outer = GetOuter(); outer; outer = outer.GetOuter())
    {
        temp = outer.GetName() + "." + temp;
    }
    UE_UClass objectClass = GetClass();
    std::string name = objectClass.GetName() + " " + temp + GetName();
    return name;
}

std::string UE_UObject::GetCppName() const
{
    std::string name;
    if (IsA<UE_UClass>())
    {
        for (auto c = Cast<UE_UStruct>(); c; c = c.GetSuper())
        {
            if (c == UE_AActor::StaticClass())
            {
                name = "A";
                break;
            }
            else if (c == UE_UObject::StaticClass())
            {
                name = "U";
                break;
            }
        }
    }
    else
    {
        name = "F";
    }

    name += GetName();
    return name;
}

bool UE_UObject::IsA(UE_UClass cmp) const
{
    for (auto super = GetClass(); super; super = super.GetSuper().Cast<UE_UClass>())
    {
        if (super == cmp)
        {
            return true;
        }
    }

    return false;
}

UE_UClass UE_UObject::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.Object"));
    return obj;
};

UE_UClass UE_AActor::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class Engine.Actor"));
    return obj;
}

UE_UField UE_UField::GetNext() const
{
    return vm_rpm_ptr<UE_UField>(object + Profile::offsets.UField.Next);
}

UE_UClass UE_UField::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.Field"));
    return obj;
};

std::string IUProperty::GetName() const
{
    return ((UE_UProperty *)(this->prop))->GetName();
}

int32 IUProperty::GetArrayDim() const
{
    return ((UE_UProperty *)(this->prop))->GetArrayDim();
}

int32 IUProperty::GetSize() const
{
    return ((UE_UProperty *)(this->prop))->GetSize();
}

int32 IUProperty::GetOffset() const
{
    return ((UE_UProperty *)(this->prop))->GetOffset();
}

uint64 IUProperty::GetPropertyFlags() const
{
    return ((UE_UProperty *)(this->prop))->GetPropertyFlags();
}

std::pair<PropertyType, std::string> IUProperty::GetType() const
{
    return ((UE_UProperty *)(this->prop))->GetType();
}

uint8 IUProperty::GetFieldMask() const
{
    return ((UE_UBoolProperty *)(this->prop))->GetFieldMask();
}

int32 UE_UProperty::GetArrayDim() const
{
    return vm_rpm_ptr<int32>(object + Profile::offsets.UProperty.ArrayDim);
}

int32 UE_UProperty::GetSize() const
{
    return vm_rpm_ptr<int32>(object + Profile::offsets.UProperty.ElementSize);
}

int32 UE_UProperty::GetOffset() const
{
    return vm_rpm_ptr<int32>(object + Profile::offsets.UProperty.Offset_Internal);
}

uint64 UE_UProperty::GetPropertyFlags() const
{
    return vm_rpm_ptr<uint64>(object + Profile::offsets.UProperty.PropertyFlags);
}

std::pair<PropertyType, std::string> UE_UProperty::GetType() const
{
    if (IsA<UE_UDoubleProperty>())
    {
        return {PropertyType::DoubleProperty, Cast<UE_UDoubleProperty>().GetTypeStr()};
    };
    if (IsA<UE_UFloatProperty>())
    {
        return {PropertyType::FloatProperty, Cast<UE_UFloatProperty>().GetTypeStr()};
    };
    if (IsA<UE_UIntProperty>())
    {
        return {PropertyType::IntProperty, Cast<UE_UIntProperty>().GetTypeStr()};
    };
    if (IsA<UE_UInt16Property>())
    {
        return {PropertyType::Int16Property, Cast<UE_UInt16Property>().GetTypeStr()};
    };
    if (IsA<UE_UInt32Property>())
    {
        return {PropertyType::Int32Property, Cast<UE_UInt32Property>().GetTypeStr()};
    }
    if (IsA<UE_UInt64Property>())
    {
        return {PropertyType::Int64Property, Cast<UE_UInt64Property>().GetTypeStr()};
    };
    if (IsA<UE_UInt8Property>())
    {
        return {PropertyType::Int8Property, Cast<UE_UInt8Property>().GetTypeStr()};
    };
    if (IsA<UE_UUInt16Property>())
    {
        return {PropertyType::UInt16Property, Cast<UE_UUInt16Property>().GetTypeStr()};
    };
    if (IsA<UE_UUInt32Property>())
    {
        return {PropertyType::UInt32Property, Cast<UE_UUInt32Property>().GetTypeStr()};
    }
    if (IsA<UE_UUInt64Property>())
    {
        return {PropertyType::UInt64Property, Cast<UE_UUInt64Property>().GetTypeStr()};
    };
    if (IsA<UE_UTextProperty>())
    {
        return {PropertyType::TextProperty, Cast<UE_UTextProperty>().GetTypeStr()};
    }
    if (IsA<UE_UStrProperty>())
    {
        return {PropertyType::TextProperty, Cast<UE_UStrProperty>().GetTypeStr()};
    };
    if (IsA<UE_UClassProperty>())
    {
        return {PropertyType::ClassProperty, Cast<UE_UClassProperty>().GetTypeStr()};
    };
    if (IsA<UE_UStructProperty>())
    {
        return {PropertyType::StructProperty, Cast<UE_UStructProperty>().GetTypeStr()};
    };
    if (IsA<UE_UNameProperty>())
    {
        return {PropertyType::NameProperty, Cast<UE_UNameProperty>().GetTypeStr()};
    };
    if (IsA<UE_UBoolProperty>())
    {
        return {PropertyType::BoolProperty, Cast<UE_UBoolProperty>().GetTypeStr()};
    }
    if (IsA<UE_UByteProperty>())
    {
        return {PropertyType::ByteProperty, Cast<UE_UByteProperty>().GetTypeStr()};
    };
    if (IsA<UE_UArrayProperty>())
    {
        return {PropertyType::ArrayProperty, Cast<UE_UArrayProperty>().GetTypeStr()};
    };
    if (IsA<UE_UEnumProperty>())
    {
        return {PropertyType::EnumProperty, Cast<UE_UEnumProperty>().GetTypeStr()};
    };
    if (IsA<UE_USetProperty>())
    {
        return {PropertyType::SetProperty, Cast<UE_USetProperty>().GetTypeStr()};
    };
    if (IsA<UE_UMapProperty>())
    {
        return {PropertyType::MapProperty, Cast<UE_UMapProperty>().GetTypeStr()};
    };
    if (IsA<UE_UInterfaceProperty>())
    {
        return {PropertyType::InterfaceProperty, Cast<UE_UInterfaceProperty>().GetTypeStr()};
    };
    if (IsA<UE_UMulticastDelegateProperty>())
    {
        return {PropertyType::MulticastDelegateProperty, Cast<UE_UMulticastDelegateProperty>().GetTypeStr()};
    };
    if (IsA<UE_UWeakObjectProperty>())
    {
        return {PropertyType::WeakObjectProperty, Cast<UE_UWeakObjectProperty>().GetTypeStr()};
    };
    if (IsA<UE_ULazyObjectProperty>())
    {
        return {PropertyType::LazyObjectProperty, Cast<UE_ULazyObjectProperty>().GetTypeStr()};
    };
    if (IsA<UE_UObjectProperty>())
    {
        return {PropertyType::ObjectProperty, Cast<UE_UObjectProperty>().GetTypeStr()};
    };
    if (IsA<UE_UObjectPropertyBase>())
    {
        return {PropertyType::ObjectProperty, Cast<UE_UObjectPropertyBase>().GetTypeStr()};
    };
    return {PropertyType::Unknown, GetClass().GetName()};
}

IUProperty UE_UProperty::GetInterface() const { return IUProperty(this); }

UE_UClass UE_UProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.Property"));
    return obj;
}

UE_UStruct UE_UStruct::GetSuper() const
{
    return vm_rpm_ptr<UE_UStruct>(object + Profile::offsets.UStruct.SuperStruct);
}

UE_FField UE_UStruct::GetChildProperties() const
{
    if (Profile::offsets.UStruct.ChildrenProps > 0)
    {
        return vm_rpm_ptr<UE_FField>(object + Profile::offsets.UStruct.ChildrenProps);
    }
    return {};
}

UE_UField UE_UStruct::GetChildren() const
{
    return vm_rpm_ptr<UE_UField>(object + Profile::offsets.UStruct.Children);
}

int32 UE_UStruct::GetSize() const
{
    return vm_rpm_ptr<int32>(object + Profile::offsets.UStruct.PropertiesSize);
};

UE_UClass UE_UStruct::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.Struct"));
    return obj;
};

uint64 UE_UFunction::GetFunc() const
{
    return vm_rpm_ptr<uint64>(object + Profile::offsets.UFunction.Func);
}

int8 UE_UFunction::GetNumParams() const
{
    return vm_rpm_ptr<int8>(object + Profile::offsets.UFunction.NumParams);
}

int16 UE_UFunction::GetParamSize() const
{
    return vm_rpm_ptr<int16>(object + Profile::offsets.UFunction.ParamSize);
}

int16 UE_UFunction::GetReturnValueOffset() const
{
    return vm_rpm_ptr<int16>(object + Profile::offsets.UFunction.ReturnValueOffset);
}

std::string UE_UFunction::GetFunctionFlags() const
{
    auto flags = vm_rpm_ptr<uint32>(object + Profile::offsets.UFunction.EFunctionFlags);
    std::string result;
    if (flags == FUNC_None)
    {
        result = "None";
    }
    else
    {
        if (flags & FUNC_Final)
        {
            result += "Final|";
        }
        if (flags & FUNC_RequiredAPI)
        {
            result += "RequiredAPI|";
        }
        if (flags & FUNC_BlueprintAuthorityOnly)
        {
            result += "BlueprintAuthorityOnly|";
        }
        if (flags & FUNC_BlueprintCosmetic)
        {
            result += "BlueprintCosmetic|";
        }
        if (flags & FUNC_Net)
        {
            result += "Net|";
        }
        if (flags & FUNC_NetReliable)
        {
            result += "NetReliable";
        }
        if (flags & FUNC_NetRequest)
        {
            result += "NetRequest|";
        }
        if (flags & FUNC_Exec)
        {
            result += "Exec|";
        }
        if (flags & FUNC_Native)
        {
            result += "Native|";
        }
        if (flags & FUNC_Event)
        {
            result += "Event|";
        }
        if (flags & FUNC_NetResponse)
        {
            result += "NetResponse|";
        }
        if (flags & FUNC_Static)
        {
            result += "Static|";
        }
        if (flags & FUNC_NetMulticast)
        {
            result += "NetMulticast|";
        }
        if (flags & FUNC_UbergraphFunction)
        {
            result += "UbergraphFunction|";
        }
        if (flags & FUNC_MulticastDelegate)
        {
            result += "MulticastDelegate|";
        }
        if (flags & FUNC_Public)
        {
            result += "Public|";
        }
        if (flags & FUNC_Private)
        {
            result += "Private|";
        }
        if (flags & FUNC_Protected)
        {
            result += "Protected|";
        }
        if (flags & FUNC_Delegate)
        {
            result += "Delegate|";
        }
        if (flags & FUNC_NetServer)
        {
            result += "NetServer|";
        }
        if (flags & FUNC_HasOutParms)
        {
            result += "HasOutParms|";
        }
        if (flags & FUNC_HasDefaults)
        {
            result += "HasDefaults|";
        }
        if (flags & FUNC_NetClient)
        {
            result += "NetClient|";
        }
        if (flags & FUNC_DLLImport)
        {
            result += "DLLImport|";
        }
        if (flags & FUNC_BlueprintCallable)
        {
            result += "BlueprintCallable|";
        }
        if (flags & FUNC_BlueprintEvent)
        {
            result += "BlueprintEvent|";
        }
        if (flags & FUNC_BlueprintPure)
        {
            result += "BlueprintPure|";
        }
        if (flags & FUNC_EditorOnly)
        {
            result += "EditorOnly|";
        }
        if (flags & FUNC_Const)
        {
            result += "Const|";
        }
        if (flags & FUNC_NetValidate)
        {
            result += "NetValidate|";
        }
        if (result.size())
        {
            result.erase(result.size() - 1);
        }
    }
    return result;
}

UE_UClass UE_UFunction::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.Function"));
    return obj;
}

UE_UClass UE_UScriptStruct::StaticClass()
{
    static UE_UClass obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.ScriptStruct"));
    return obj;
};

UE_UClass UE_UClass::StaticClass()
{
    static UE_UClass obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.Class"));
    return obj;
};

TArray UE_UEnum::GetNames() const
{
    return vm_rpm_ptr<TArray>(object + Profile::offsets.UEnum.Names);
}

UE_UClass UE_UEnum::StaticClass()
{
    static UE_UClass obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.Enum"));
    return obj;
}

std::string UE_UDoubleProperty::GetTypeStr() const { return "double"; }

UE_UClass UE_UDoubleProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.DoubleProperty"));
    return obj;
}

UE_UStruct UE_UStructProperty::GetStruct() const
{
    return vm_rpm_ptr<UE_UStruct>(object + Profile::offsets.UProperty.Size);
}

std::string UE_UStructProperty::GetTypeStr() const
{
    return "struct " + GetStruct().GetCppName();
}

UE_UClass UE_UStructProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.StructProperty"));
    return obj;
}

std::string UE_UNameProperty::GetTypeStr() const { return "struct FName"; }

UE_UClass UE_UNameProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.NameProperty"));
    return obj;
}

UE_UClass UE_UObjectPropertyBase::GetPropertyClass() const
{
    return vm_rpm_ptr<UE_UClass>(object + Profile::offsets.UProperty.Size);
}

std::string UE_UObjectPropertyBase::GetTypeStr() const
{
    return "struct " + GetPropertyClass().GetCppName() + "*";
}

UE_UClass UE_UObjectPropertyBase::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.ObjectPropertyBase"));
    return obj;
}

UE_UClass UE_UObjectProperty::GetPropertyClass() const
{
    return vm_rpm_ptr<UE_UClass>(object + Profile::offsets.UProperty.Size);
}

std::string UE_UObjectProperty::GetTypeStr() const
{
    return "struct " + GetPropertyClass().GetCppName() + "*";
}

UE_UClass UE_UObjectProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.ObjectProperty"));
    return obj;
}

UE_UProperty UE_UArrayProperty::GetInner() const
{
    return vm_rpm_ptr<UE_UProperty>(object + Profile::offsets.UProperty.Size);
}

std::string UE_UArrayProperty::GetTypeStr() const
{
    return "struct TArray<" + GetInner().GetType().second + ">";
}

UE_UClass UE_UArrayProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.ArrayProperty"));
    return obj;
}

UE_UEnum UE_UByteProperty::GetEnum() const
{
    return vm_rpm_ptr<UE_UEnum>(object + Profile::offsets.UProperty.Size);
}

std::string UE_UByteProperty::GetTypeStr() const
{
    auto e = GetEnum();
    if (e)
        return "enum class " + e.GetName();
    return "char";
}

UE_UClass UE_UByteProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.ByteProperty"));
    return obj;
}

uint8 UE_UBoolProperty::GetFieldMask() const
{
    return vm_rpm_ptr<uint8>(object + Profile::offsets.UProperty.Size + 3);
}

std::string UE_UBoolProperty::GetTypeStr() const
{
    if (GetFieldMask() == 0xFF)
    {
        return "bool";
    };
    return "char";
}

UE_UClass UE_UBoolProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.BoolProperty"));
    return obj;
}

std::string UE_UFloatProperty::GetTypeStr() const { return "float"; }

UE_UClass UE_UFloatProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.FloatProperty"));
    return obj;
}

std::string UE_UIntProperty::GetTypeStr() const { return "int"; }

UE_UClass UE_UIntProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.IntProperty"));
    return obj;
}

std::string UE_UInt16Property::GetTypeStr() const { return "int16_t"; }

UE_UClass UE_UInt16Property::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.Int16Property"));
    return obj;
}

std::string UE_UInt64Property::GetTypeStr() const { return "int64_t"; }

UE_UClass UE_UInt64Property::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.Int64Property"));
    return obj;
}

std::string UE_UInt8Property::GetTypeStr() const { return "uint8_t"; }

UE_UClass UE_UInt8Property::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.Int8Property"));
    return obj;
}

std::string UE_UUInt16Property::GetTypeStr() const { return "uint16_t"; }

UE_UClass UE_UUInt16Property::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.UInt16Property"));
    return obj;
}

std::string UE_UUInt32Property::GetTypeStr() const { return "uint32_t"; }

UE_UClass UE_UUInt32Property::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.UInt32Property"));
    return obj;
}

std::string UE_UInt32Property::GetTypeStr() const { return "int32_t"; }

UE_UClass UE_UInt32Property::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.Int32Property"));
    return obj;
}

std::string UE_UUInt64Property::GetTypeStr() const { return "uint64"; }

UE_UClass UE_UUInt64Property::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.UInt64Property"));
    return obj;
}

std::string UE_UTextProperty::GetTypeStr() const { return "struct FText"; }

UE_UClass UE_UTextProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.TextProperty"));
    return obj;
}

std::string UE_UStrProperty::GetTypeStr() const { return "struct FString"; }

UE_UClass UE_UStrProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.StrProperty"));
    return obj;
}

UE_UClass UE_UEnumProperty::GetEnum() const
{
    return vm_rpm_ptr<UE_UClass>(object + Profile::offsets.UProperty.Size + 8);
}

std::string UE_UEnumProperty::GetTypeStr() const
{
    return "enum class " + GetEnum().GetName();
}

UE_UClass UE_UEnumProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.EnumProperty"));
    return obj;
}

UE_UClass UE_UClassProperty::GetMetaClass() const
{
    return vm_rpm_ptr<UE_UClass>(object + Profile::offsets.UProperty.Size + 8);
}

std::string UE_UClassProperty::GetTypeStr() const
{
    return "struct " + GetMetaClass().GetCppName() + "*";
}

UE_UClass UE_UClassProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.ClassProperty"));
    return obj;
}

UE_UProperty UE_USetProperty::GetElementProp() const
{
    return vm_rpm_ptr<UE_UProperty>(object + Profile::offsets.UProperty.Size);
}

std::string UE_USetProperty::GetTypeStr() const
{
    return "struct TSet<" + GetElementProp().GetType().second + ">";
}

UE_UClass UE_USetProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.SetProperty"));
    return obj;
}

UE_UProperty UE_UMapProperty::GetKeyProp() const
{
    return vm_rpm_ptr<UE_UProperty>(object + Profile::offsets.UProperty.Size);
}

UE_UProperty UE_UMapProperty::GetValueProp() const
{
    return vm_rpm_ptr<UE_UProperty>(object + Profile::offsets.UProperty.Size + 8);
}

std::string UE_UMapProperty::GetTypeStr() const
{
    return fmt::format("struct TMap<{}, {}>", GetKeyProp().GetType().second, GetValueProp().GetType().second);
}

UE_UClass UE_UMapProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.MapProperty"));
    return obj;
}

UE_UProperty UE_UInterfaceProperty::GetInterfaceClass() const
{
    return vm_rpm_ptr<UE_UProperty>(object + Profile::offsets.UProperty.Size);
}

std::string UE_UInterfaceProperty::GetTypeStr() const
{
    return "struct TScriptInterface<" + GetInterfaceClass().GetType().second + ">";
}

UE_UClass UE_UInterfaceProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.InterfaceProperty"));
    return obj;
}

std::string UE_UMulticastDelegateProperty::GetTypeStr() const
{
    return "struct FScriptMulticastDelegate";
}

UE_UClass UE_UMulticastDelegateProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.MulticastDelegateProperty"));
    return obj;
}

std::string UE_UWeakObjectProperty::GetTypeStr() const
{
    return "struct TWeakObjectPtr<" + this->Cast<UE_UStructProperty>().GetTypeStr() + ">";
}

UE_UClass UE_UWeakObjectProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.WeakObjectProperty"));
    return obj;
}

std::string UE_ULazyObjectProperty::GetTypeStr() const
{
    return "struct TLazyObjectPtr<" + this->Cast<UE_UStructProperty>().GetTypeStr() + ">";
}

UE_UClass UE_ULazyObjectProperty::StaticClass()
{
    static auto obj = (UE_UClass)(Profile::ObjObjects.FindObject("Class CoreUObject.LazyObjectProperty"));
    return obj;
}

std::string UE_FFieldClass::GetName() const
{
    auto name = UE_FName(object);
    return name.GetName();
}

UE_FField UE_FField::GetNext() const
{
    return vm_rpm_ptr<UE_FField>(object + Profile::offsets.FField.Next);
};

std::string UE_FField::GetName() const
{
    auto name = UE_FName(object + Profile::offsets.FField.NamePrivate);
    return name.GetName();
}

std::string IFProperty::GetName() const
{
    return ((UE_FProperty *)prop)->GetName();
}

int32 IFProperty::GetArrayDim() const
{
    return ((UE_FProperty *)prop)->GetArrayDim();
}

int32 IFProperty::GetSize() const { return ((UE_FProperty *)prop)->GetSize(); }

int32 IFProperty::GetOffset() const
{
    return ((UE_FProperty *)prop)->GetOffset();
}

uint64 IFProperty::GetPropertyFlags() const
{
    return ((UE_FProperty *)prop)->GetPropertyFlags();
}

std::pair<PropertyType, std::string> IFProperty::GetType() const
{
    return ((UE_FProperty *)prop)->GetType();
}

uint8 IFProperty::GetFieldMask() const
{
    return ((UE_FBoolProperty *)prop)->GetFieldMask();
}

int32 UE_FProperty::GetArrayDim() const
{
    return vm_rpm_ptr<int32>(object + Profile::offsets.FProperty.ArrayDim);
}

int32 UE_FProperty::GetSize() const
{
    return vm_rpm_ptr<int32>(object + Profile::offsets.FProperty.ElementSize);
}

int32 UE_FProperty::GetOffset() const
{
    return vm_rpm_ptr<int32>(object + Profile::offsets.FProperty.Offset_Internal);
}

uint64 UE_FProperty::GetPropertyFlags() const
{
    return vm_rpm_ptr<uint64>(object + Profile::offsets.FProperty.PropertyFlags);
}

type UE_FProperty::GetType() const
{
    auto objectClass = vm_rpm_ptr<UE_FFieldClass>(object + Profile::offsets.FField.ClassPrivate);
    type type = {PropertyType::Unknown, objectClass.GetName()};

    auto &str = type.second;
    auto hash = Hash(str.c_str(), str.size());
    switch (hash)
    {
    case HASH("StructProperty"):
    {
        auto obj = this->Cast<UE_FStructProperty>();
        type = {PropertyType::StructProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("ObjectProperty"):
    {
        auto obj = this->Cast<UE_FObjectPropertyBase>();
        type = {PropertyType::ObjectProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("SoftObjectProperty"):
    {
        auto obj = this->Cast<UE_FObjectPropertyBase>();
        type = {PropertyType::SoftObjectProperty, "struct TSoftObjectPtr<" + obj.GetPropertyClass().GetCppName() + ">"};
        break;
    }
    case HASH("FloatProperty"):
    {
        type = {PropertyType::FloatProperty, "float"};
        break;
    }
    case HASH("ByteProperty"):
    {
        auto obj = this->Cast<UE_FByteProperty>();
        type = {PropertyType::ByteProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("BoolProperty"):
    {
        auto obj = this->Cast<UE_FBoolProperty>();
        type = {PropertyType::BoolProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("IntProperty"):
    {
        type = {PropertyType::IntProperty, "int32_t"};
        break;
    }
    case HASH("Int8Property"):
    {
        type = {PropertyType::Int8Property, "int8_t"};
        break;
    }
    case HASH("Int16Property"):
    {
        type = {PropertyType::Int16Property, "int16_t"};
        break;
    }
    case HASH("Int64Property"):
    {
        type = {PropertyType::Int64Property, "int64_t"};
        break;
    }
    case HASH("UInt16Property"):
    {
        type = {PropertyType::UInt16Property, "uint16_t"};
        break;
    }
    case HASH("Int32Property"):
    {
        type = {PropertyType::Int32Property, "int32_t"};
        break;
    }
    case HASH("UInt32Property"):
    {
        type = {PropertyType::UInt32Property, "uint32_t"};
        break;
    }
    case HASH("UInt64Property"):
    {
        type = {PropertyType::UInt64Property, "uint64_t"};
        break;
    }
    case HASH("NameProperty"):
    {
        type = {PropertyType::NameProperty, "struct FName"};
        break;
    }
    case HASH("DelegateProperty"):
    {
        type = {PropertyType::DelegateProperty, "struct FDelegate"};
        break;
    }
    case HASH("SetProperty"):
    {
        auto obj = this->Cast<UE_FSetProperty>();
        type = {PropertyType::SetProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("ArrayProperty"):
    {
        auto obj = this->Cast<UE_FArrayProperty>();
        type = {PropertyType::ArrayProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("WeakObjectProperty"):
    {
        auto obj = this->Cast<UE_FStructProperty>();
        type = {PropertyType::WeakObjectProperty, "struct TWeakObjectPtr<" + obj.GetTypeStr() + ">"};
        break;
    }
    case HASH("StrProperty"):
    {
        type = {PropertyType::StrProperty, "struct FString"};
        break;
    }
    case HASH("TextProperty"):
    {
        type = {PropertyType::TextProperty, "struct FText"};
        break;
    }
    case HASH("MulticastSparseDelegateProperty"):
    {
        type = {PropertyType::MulticastSparseDelegateProperty, "struct FMulticastSparseDelegate"};
        break;
    }
    case HASH("EnumProperty"):
    {
        auto obj = this->Cast<UE_FEnumProperty>();
        type = {PropertyType::EnumProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("DoubleProperty"):
    {
        type = {PropertyType::DoubleProperty, "double"};
        break;
    }
    case HASH("MulticastDelegateProperty"):
    {
        type = {PropertyType::MulticastDelegateProperty, "FMulticastDelegate"};
        break;
    }
    case HASH("ClassProperty"):
    {
        auto obj = this->Cast<UE_FClassProperty>();
        type = {PropertyType::ClassProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("MulticastInlineDelegateProperty"):
    {
        type = {PropertyType::MulticastDelegateProperty, "struct FMulticastInlineDelegate"};
        break;
    }
    case HASH("MapProperty"):
    {
        auto obj = this->Cast<UE_FMapProperty>();
        type = {PropertyType::MapProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("InterfaceProperty"):
    {
        auto obj = this->Cast<UE_FInterfaceProperty>();
        type = {PropertyType::InterfaceProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("FieldPathProperty"):
    {
        auto obj = this->Cast<UE_FFieldPathProperty>();
        type = {PropertyType::FieldPathProperty, obj.GetTypeStr()};
        break;
    }
    case HASH("SoftClassProperty"):
    {
        type = {PropertyType::SoftClassProperty, "struct TSoftClassPtr<UObject>"};
        break;
    }
    }

    return type;
}

IFProperty UE_FProperty::GetInterface() const { return IFProperty(this); }

UE_UStruct UE_FStructProperty::GetStruct() const
{
    return vm_rpm_ptr<UE_UStruct>(object + Profile::offsets.FProperty.Size);
}

std::string UE_FStructProperty::GetTypeStr() const
{
    return "struct " + GetStruct().GetCppName();
}

UE_UClass UE_FObjectPropertyBase::GetPropertyClass() const
{
    return vm_rpm_ptr<UE_UClass>(object + Profile::offsets.FProperty.Size);
}

std::string UE_FObjectPropertyBase::GetTypeStr() const
{
    return "struct " + GetPropertyClass().GetCppName() + "*";
}

UE_FProperty UE_FArrayProperty::GetInner() const
{
    return vm_rpm_ptr<UE_FProperty>(object + Profile::offsets.FProperty.Size);
}

std::string UE_FArrayProperty::GetTypeStr() const
{
    return "struct TArray<" + GetInner().GetType().second + ">";
}

UE_UEnum UE_FByteProperty::GetEnum() const
{
    return vm_rpm_ptr<UE_UEnum>(object + Profile::offsets.FProperty.Size);
}

std::string UE_FByteProperty::GetTypeStr() const
{
    auto e = GetEnum();
    if (e)
        return "enum class " + e.GetName();
    return "char";
}

uint8 UE_FBoolProperty::GetFieldMask() const
{
    return vm_rpm_ptr<uint8>(object + Profile::offsets.FProperty.Size + 3);
}

std::string UE_FBoolProperty::GetTypeStr() const
{
    if (GetFieldMask() == 0xFF)
    {
        return "bool";
    };
    return "char";
}

UE_UClass UE_FEnumProperty::GetEnum() const
{
    return vm_rpm_ptr<UE_UClass>(object + Profile::offsets.FProperty.Size + 8);
}

std::string UE_FEnumProperty::GetTypeStr() const
{
    return "enum class " + GetEnum().GetName();
}

UE_UClass UE_FClassProperty::GetMetaClass() const
{
    return vm_rpm_ptr<UE_UClass>(object + Profile::offsets.FProperty.Size + 8);
}

std::string UE_FClassProperty::GetTypeStr() const
{
    return "struct " + GetMetaClass().GetCppName() + "*";
}

UE_FProperty UE_FSetProperty::GetElementProp() const
{
    return vm_rpm_ptr<UE_FProperty>(object + Profile::offsets.FProperty.Size);
}

std::string UE_FSetProperty::GetTypeStr() const
{
    return "struct TSet<" + GetElementProp().GetType().second + ">";
}

UE_FProperty UE_FMapProperty::GetKeyProp() const
{
    return vm_rpm_ptr<UE_FProperty>(object + Profile::offsets.FProperty.Size);
}

UE_FProperty UE_FMapProperty::GetValueProp() const
{
    return vm_rpm_ptr<UE_FProperty>(object + Profile::offsets.FProperty.Size + 8);
}

std::string UE_FMapProperty::GetTypeStr() const
{
    return fmt::format("struct TMap<{}, {}>", GetKeyProp().GetType().second, GetValueProp().GetType().second);
}

UE_UClass UE_FInterfaceProperty::GetInterfaceClass() const
{
    return vm_rpm_ptr<UE_UClass>(object + Profile::offsets.FProperty.Size);
}

std::string UE_FInterfaceProperty::GetTypeStr() const
{
    return "struct TScriptInterface<I" + GetInterfaceClass().GetName() + ">";
}

UE_FName UE_FFieldPathProperty::GetPropertyName() const
{
    return vm_rpm_ptr<UE_FName>(object + Profile::offsets.FProperty.Size);
}

std::string UE_FFieldPathProperty::GetTypeStr() const
{
    return "struct TFieldPath<F" + GetPropertyName().GetName() + ">";
}