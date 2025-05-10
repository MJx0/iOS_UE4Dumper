#include "Dumper.hpp"

#include <fmt/format.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "wrappers.hpp"

namespace Dumper
{
    namespace jf_ns
    {
        struct JsonFunction
        {
            std::string Parent;
            std::string Name;
            uint64_t Address = 0;
        };
        static std::vector<JsonFunction> jsonFunctions;

        void to_json(json &j, const JsonFunction &jf)
        {
            if (jf.Parent.empty() || jf.Parent == "None" || jf.Parent == "null")
                return;
            if (jf.Name.empty() || jf.Name == "None" || jf.Name == "null")
                return;
            if (jf.Address == 0)
                return;

            std::string fname = ioutils::replace_specials(jf.Parent, '_');
            fname += "$$";
            fname += ioutils::replace_specials(jf.Name, '_');

            j = json{{"Name", fname}, {"Address", (jf.Address - UEVars::BaseAddress) - UEVars::pagezero_size}};
        }
    } // namespace jf_ns

    DumpStatus InitUEVars(IGameProfile *gameProfile)
    {
        UEVars::Profile = gameProfile;

        UEVars::BaseAddress = gameProfile->GetExecutableInfo().address;
        if (UEVars::BaseAddress == 0)
            return UE_DS_ERROR_EXE_NOT_FOUND;

        UEVars::pagezero_size = gameProfile->GetExecutableInfo().getSegment("__PAGEZERO").size;

        UE_Offsets *p_offsets = gameProfile->GetOffsets();
        if (!p_offsets)
            return UE_DS_ERROR_INIT_OFFSETS;

        UEVars::Offsets = *(UE_Offsets *)p_offsets;

        UEVars::isUsingFNamePool = gameProfile->IsUsingFNamePool();
        UEVars::isUsingOutlineNumberName = gameProfile->isUsingOutlineNumberName();

        if (UEVars::isUsingFNamePool)
        {
            UEVars::NamePoolDataPtr = gameProfile->GetNamesPtr();
            if (UEVars::NamePoolDataPtr == 0)
                return UE_DS_ERROR_INIT_NAMEPOOL;
        }
        else
        {
            UEVars::GNamesPtr = gameProfile->GetNamesPtr();
            if (UEVars::GNamesPtr == 0)
                return UE_DS_ERROR_INIT_GNAMES;
        }

        uintptr_t GUObjectsArrayPtr = gameProfile->GetGUObjectArrayPtr();
        if (GUObjectsArrayPtr == 0)
            return UE_DS_ERROR_INIT_GUOBJECTARRAY;

        UEVars::ObjObjectsPtr = GUObjectsArrayPtr + UEVars::Offsets.FUObjectArray.ObjObjects;

        if (!vm_rpm_ptr((void *)(UEVars::ObjObjectsPtr + UEVars::Offsets.TUObjectArray.Objects), &UEVars::ObjObjects.Objects, sizeof(uintptr_t)))
            return UE_DS_ERROR_INIT_OBJOBJECTS;

        return UE_DS_NONE;
    }

    DumpStatus Dump(std::unordered_map<std::string, BufferFmt> *outBuffersMap)
    {

        auto exe_info = UEVars::Profile->GetExecutableInfo();
        if (!exe_info.name || exe_info.address == 0)
            return UE_DS_ERROR_EXE_NOT_FOUND;

        outBuffersMap->insert({"Logs.txt", BufferFmt()});
        BufferFmt &logsBufferFmt = outBuffersMap->at("Logs.txt");

        logsBufferFmt.append("Executable: {}\n", exe_info.name);
        if (UEVars::pagezero_size > 0)
        {
            logsBufferFmt.append("__PAGEZERO Size: 0x{:X}\n", UEVars::pagezero_size);
        }
        else
        {
            logsBufferFmt.append("__PAGEZERO not available\n");
        }
        logsBufferFmt.append("BaseAddress: 0x{:X}\n", UEVars::BaseAddress);
        logsBufferFmt.append("==========================\n");

        if (!UEVars::isUsingFNamePool)
        {
            logsBufferFmt.append("GNames: [<Base> + 0x{:X}] = 0x{:X}\n",
                                 UEVars::GNamesPtr - UEVars::BaseAddress, UEVars::GNamesPtr);
        }
        else
        {
            logsBufferFmt.append("FNamePool: [<Base> + 0x{:X}] = 0x{:X}\n",
                                 UEVars::NamePoolDataPtr - UEVars::BaseAddress, UEVars::NamePoolDataPtr);
        }

        logsBufferFmt.append("Test dumping first 5 name entries\n");
        for (int i = 0; i < 5; i++)
        {
            logsBufferFmt.append("GetNameByID({}): {}\n", i, UEVars::Profile->GetNameByID(i));
        }
        logsBufferFmt.append("==========================\n");

        logsBufferFmt.append("ObjObjects: [<Base> + 0x{:X}] = 0x{:X}\n", UEVars::ObjObjectsPtr - UEVars::BaseAddress, UEVars::ObjObjectsPtr);
        logsBufferFmt.append("ObjObjects Num: {}\n", UEVars::ObjObjects.GetNumElements());
        logsBufferFmt.append("Test Dumping First 5 Name Entries\n");
        for (int i = 0; i < 5; i++)
        {
            UE_UObject obj = UEVars::ObjObjects.GetObjectPtr(i);
            logsBufferFmt.append("GetObjectPtr({}): {}\n", i, obj.GetName());
        }
        logsBufferFmt.append("==========================\n");

        outBuffersMap->insert({"Objects.txt", BufferFmt()});
        BufferFmt &objsBufferFmt = outBuffersMap->at("Objects.txt");

        std::vector<std::pair<uint8_t *const, std::vector<UE_UObject>>> packages;
        std::function<bool(UE_UObject)> callback;

        callback = [&objsBufferFmt, &packages](UE_UObject object)
        {
            if (object.IsA<UE_UFunction>() || object.IsA<UE_UStruct>() || object.IsA<UE_UEnum>())
            {
                bool found = false;
                auto packageObj = object.GetPackageObject();
                for (auto &pkg : packages)
                {
                    if (pkg.first == packageObj)
                    {
                        pkg.second.push_back(object);
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    packages.push_back(std::make_pair(packageObj, std::vector<UE_UObject>(1, object)));
                }
            }

            objsBufferFmt.append("[{:010}]: {}\n", object.GetIndex(), object.GetFullName());

            return false;
        };

        UEVars::ObjObjects.ForEachObject(callback);

        if (!packages.size())
        {
            logsBufferFmt.append("Error: Packages are empty.\n");
            return UE_DS_ERROR_EMPTY_PACKAGES;
        }

        int packages_saved = 0;
        std::string packages_unsaved{};

        int classes_saved = 0;
        int structs_saved = 0;
        int enums_saved = 0;

        static bool processInternal_once = false;

        outBuffersMap->insert({"AIOHeader.hpp", BufferFmt()});
        BufferFmt &aioBufferFmt = outBuffersMap->at("AIOHeader.hpp");

        for (UE_UPackage package : packages)
        {
            package.Process();

            if (!package.AppendToBuffer(&aioBufferFmt))
            {
                packages_unsaved += "\t";
                packages_unsaved += (package.GetObject().GetName() + ",\n");
                continue;
            }

            packages_saved++;
            classes_saved += package.Classes.size();
            structs_saved += package.Structures.size();
            enums_saved += package.Enums.size();

            for (const auto &cls : package.Classes)
            {
                for (const auto &func : cls.Functions)
                {
                    // UObject::ProcessInternal for blueprint functions
                    if (!processInternal_once && (func.EFlags & FUNC_BlueprintEvent) && func.Func)
                    {
                        jf_ns::jsonFunctions.push_back({"UObject", "ProcessInternal", func.Func});
                        processInternal_once = true;
                    }

                    if ((func.EFlags & FUNC_Native) && func.Func)
                    {
                        std::string execFuncName = "exec";
                        execFuncName += func.Name;
                        jf_ns::jsonFunctions.push_back({cls.Name, execFuncName, func.Func});
                    }
                }
            }

            for (const auto &st : package.Structures)
            {
                for (const auto &func : st.Functions)
                {
                    if ((func.EFlags & FUNC_Native) && func.Func)
                    {
                        std::string execFuncName = "exec";
                        execFuncName += func.Name;
                        jf_ns::jsonFunctions.push_back({st.Name, execFuncName, func.Func});
                    }
                }
            }
        }

        logsBufferFmt.append("Saved packages: {}\nSaved classes: {}\nSaved structs: {}\nSaved enums: {}\n", packages_saved, classes_saved, structs_saved, enums_saved);

        if (packages_unsaved.size())
        {
            packages_unsaved.erase(packages_unsaved.size() - 2);
            logsBufferFmt.append("Unsaved packages: [\n{}\n]\n", packages_unsaved);
        }

        logsBufferFmt.append("==========================\n");

        if (jf_ns::jsonFunctions.size())
        {
            outBuffersMap->insert({"script.json", BufferFmt()});
            BufferFmt &scriptBufferFmt = outBuffersMap->at("script.json");

            scriptBufferFmt.append("Generating script json...\nFunctions: {}\n", jf_ns::jsonFunctions.size());

            json js;
            for (const auto &jf : jf_ns::jsonFunctions)
            {
                js["Functions"].push_back(jf);
            }

            scriptBufferFmt.append("{}", js.dump(4));
        }

        return UE_DS_SUCCESS;
    }

    std::string DumpStatusToStr(DumpStatus ds)
    {
        switch (ds)
        {
        case UE_DS_NONE:
            return "DS_NONE";
        case UE_DS_SUCCESS:
            return "DS_SUCCESS";
        case UE_DS_ERROR_EXE_NAME_NULL:
            return "DS_ERROR_EXE_NAME_NULL";
        case UE_DS_ERROR_EXE_NOT_FOUND:
            return "DS_ERROR_EXE_NOT_FOUND";
        case UE_DS_ERROR_INIT_GNAMES:
            return "DS_ERROR_INIT_GNAMES";
        case UE_DS_ERROR_INIT_NAMEPOOL:
            return "DS_ERROR_INIT_NAMEPOOL";
        case UE_DS_ERROR_INIT_GUOBJECTARRAY:
            return "DS_ERROR_INIT_GUOBJECTARRAY";
        case UE_DS_ERROR_INIT_OBJOBJECTS:
            return "DS_ERROR_INIT_OBJOBJECTS";
        case UE_DS_ERROR_INIT_OFFSETS:
            return "DS_ERROR_INIT_OFFSETS";
        case UE_DS_ERROR_EMPTY_PACKAGES:
            return "DS_ERROR_EMPTY_PACKAGES";
        default:
            break;
        }
        return "UNKNOWN";
    }

} // namespace Dumper
