#include "Dumper.hpp"

#include <fmt/core.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "wrappers.hpp"

namespace Dumper
{

	namespace JsonGen
	{
		struct IdaFunction
		{
			std::string Parent;
			std::string Name;
			uint64 Address = 0;
		};

		static std::vector<IdaFunction> idaFunctions;
		static unsigned long __pagezero_size = 0;

		void to_json(json &j, const IdaFunction &f)
		{
			if (f.Parent.empty() || f.Parent == "None" || f.Parent == "null")
				return;
			if (f.Name.empty() || f.Name == "None" || f.Name == "null")
				return;
			if (f.Address == 0)
				return;

			std::string fname = ioutils::replace_specials(f.Parent, '_');
			fname += "$$";
			fname += ioutils::replace_specials(f.Name, '_');

			j = json{{"Name", fname}, {"Address", f.Address - __pagezero_size}};
		}
	};

	DumpStatus InitProfile(IGameProfile *gameProfile)
	{
		Profile::BaseAddress = gameProfile->GetExecutableInfo().address;
		if (Profile::BaseAddress == 0)
			return UE_DS_ERROR_EXE_NOT_FOUND;

		GetSegmentData(gameProfile->GetExecutableInfo().header, "__PAGEZERO", &JsonGen::__pagezero_size);

		UE_Offsets *p_offsets = gameProfile->GetOffsets();
		if (!p_offsets)
			return UE_DS_ERROR_INIT_OFFSETS;

		Profile::offsets = *(UE_Offsets *)p_offsets;

		Profile::isUsingFNamePool = gameProfile->IsUsingFNamePool();

		if (Profile::isUsingFNamePool)
		{
			Profile::NamePoolDataPtr = gameProfile->GetNamesPtr();
			if (Profile::NamePoolDataPtr == 0)
				return UE_DS_ERROR_INIT_NAMEPOOL;
		}
		else
		{
			Profile::GNamesPtr = gameProfile->GetNamesPtr();
			if (Profile::GNamesPtr == 0)
				return UE_DS_ERROR_INIT_GNAMES;
		}

		uintptr_t GUObjectsArrayPtr = gameProfile->GetGUObjectArrayPtr();
		if (GUObjectsArrayPtr == 0)
			return UE_DS_ERROR_INIT_GUOBJECTARRAY;

		Profile::ObjObjectsPtr = GUObjectsArrayPtr + Profile::offsets.FUObjectArray.ObjObjects;
		if (!vm_rpm_ptr((void *)Profile::ObjObjectsPtr, &Profile::ObjObjects, sizeof(TUObjectArray)))
			return UE_DS_ERROR_INIT_GUOBJECTARRAY;

		return UE_DS_NONE;
	}

	DumpStatus Dump(const std::string &dir, const std::string &headers_dir, IGameProfile *profile)
	{
		auto exe_info = profile->GetExecutableInfo();
		if (!exe_info.name || exe_info.address == 0)
			return UE_DS_ERROR_EXE_NOT_FOUND;

		auto profile_init_status = InitProfile(profile);
		if (profile_init_status != UE_DS_NONE)
			return profile_init_status;

		File logfile(dir + "/logs.txt", "w");
		if (!logfile.ok())
			return UE_DS_ERROR_IO_OPERATION;

		fmt::print(logfile, "Executable: {}\n", exe_info.name);
		if (JsonGen::__pagezero_size > 0)
		{
			fmt::print(logfile, "__PAGEZERO Size: {:#x}\n", JsonGen::__pagezero_size);
		}
		else
		{
			fmt::print(logfile, "__PAGEZERO not available\n");
		}
		fmt::print(logfile, "BaseAddress: {:#08x}\n", Profile::BaseAddress);
		fmt::print(logfile, "==========================\n");

		if (!Profile::isUsingFNamePool)
		{
			fmt::print(logfile, "GNames: [Base + {:#08x}] = {:#08x}\n", Profile::GNamesPtr-Profile::BaseAddress, Profile::GNamesPtr);
		}
		else
		{
			fmt::print(logfile, "FNamePool: [Base + {:#08x}] = {:#08x}\n", Profile::NamePoolDataPtr-Profile::BaseAddress, Profile::NamePoolDataPtr);
		}
		fmt::print(logfile, "Test Dumping First 5 Name Enteries\n");
		for (int i = 0; i < 5; i++)
		{
			std::string name = GetNameByID(i);
			if (name.empty()) continue;

			fmt::print(logfile, "GetNameByID({}): {}\n", i, GetNameByID(i));
		}
		fmt::print(logfile, "==========================\n");

		fmt::print(logfile, "ObjObjects: [Base + {:#08x}] = {:#08x}\n", Profile::ObjObjectsPtr-Profile::BaseAddress, Profile::ObjObjectsPtr);
		fmt::print(logfile, "ObjObjects Num: {}\n", Profile::ObjObjects.GetNumElements());
		fmt::print(logfile, "ObjObjects Max: {}\n", Profile::ObjObjects.GetMaxElements());
		fmt::print(logfile, "==========================\n");

		File objfile(dir + "/ObjectsDump.txt", "w");
		if (!objfile.ok())
			return UE_DS_ERROR_IO_OPERATION;

		std::function<void(UE_UObject)> objdump_callback;
		objdump_callback = [&objfile](UE_UObject object)
		{
			fmt::print(objfile, "[{:010}]: {}\n", object.GetIndex(), object.GetFullName());
		};

		std::unordered_map<uint8 *, std::vector<UE_UObject>> packages;
		std::function<void(UE_UObject)> callback;
		callback = [&objdump_callback, &packages](UE_UObject object)
		{
			if (object.IsA<UE_UFunction>() || object.IsA<UE_UStruct>() || object.IsA<UE_UEnum>())
			{
				auto packageObj = object.GetPackageObject();
				packages[packageObj].push_back(object);
			}
			if (objdump_callback)
				objdump_callback(object);
		};

		Profile::ObjObjects.ForEachObject(callback);

		if (!packages.size())
		{
			fmt::print(logfile, "Error: Packages are empty.\n");
			return UE_DS_ERROR_EMPTY_PACKAGES;
		}

		int packages_saved = 0;
		std::string packages_unsaved{};

		int classes_saved = 0;
		int structs_saved = 0;
		int enums_saved = 0;

		static bool processInternal_once = false;
		
		for (UE_UPackage package : packages)
		{
			package.Process();
			if (package.Save(dir, headers_dir))
			{
				packages_saved++;
				classes_saved += package.Classes.size();
				structs_saved += package.Structures.size();
				enums_saved += package.Enums.size();

				for (const auto &cls : package.Classes)
				{
					if (!cls.Functions.size())
						continue;

					for (const auto &func : cls.Functions)
					{
						// UObject::ProcessInternal for blueprint functions
						if(!processInternal_once && (func.EFlags & FUNC_BlueprintEvent) && func.Func)
						{
							JsonGen::idaFunctions.push_back({"UObject", "ProcessInternal", func.Func - Profile::BaseAddress});
							processInternal_once = true;
						}

						if ((func.EFlags & FUNC_Native) && func.Func)
						{
							std::string execFuncName = "exec";
							execFuncName += func.Name;
							JsonGen::idaFunctions.push_back({cls.Name, execFuncName, func.Func - Profile::BaseAddress});
						}
					}
				}

				for (const auto &st : package.Structures)
				{
					if (!st.Functions.size())
						continue;

					for (const auto &func : st.Functions)
					{
						if ((func.EFlags & FUNC_Native) && func.Func)
						{
							std::string execFuncName = "exec";
							execFuncName += func.Name;
							JsonGen::idaFunctions.push_back({st.Name, execFuncName, func.Func - Profile::BaseAddress});
						}
					}
				}
			}
			else
			{
				packages_unsaved += "\t";
				packages_unsaved += (package.GetObject().GetName() + ",\n");
			}
		}

		fmt::print(logfile, "Saved packages: {}\nSaved classes: {}\nSaved structs: {}\nSaved enums: {}\n", packages_saved, classes_saved, structs_saved, enums_saved);

		if (packages_unsaved.size())
		{
			packages_unsaved.erase(packages_unsaved.size() - 2);
			fmt::print(logfile, "Unsaved packages: [\n{}\n]\n", packages_unsaved);
		}

		fmt::print(logfile, "==========================\n");

		if (JsonGen::idaFunctions.size())
		{
			fmt::print(logfile, "Generating json...\nFunctions: {}\n", JsonGen::idaFunctions.size());
			json js;
			for (auto &idf : JsonGen::idaFunctions)
			{
				js["Functions"].push_back(idf);
			}

			File jsfile(dir + "/script.json", "w");
			if (!jsfile.ok())
			    return UE_DS_ERROR_IO_OPERATION;

			fmt::print(jsfile, "{}", js.dump(4));
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
		case UE_DS_ERROR_IO_OPERATION:
			return "DS_ERROR_IO_OPERATION";
		case UE_DS_ERROR_INIT_GNAMES:
			return "DS_ERROR_INIT_GNAMES";
		case UE_DS_ERROR_INIT_NAMEPOOL:
			return "DS_ERROR_INIT_NAMEPOOL";
		case UE_DS_ERROR_INIT_GUOBJECTARRAY:
			return "DS_ERROR_INIT_GUOBJECTARRAY";
		case UE_DS_ERROR_INIT_OFFSETS:
			return "DS_ERROR_INIT_OFFSETS";
		case UE_DS_ERROR_EMPTY_PACKAGES:
			return "DS_ERROR_EMPTY_PACKAGES";
		default:
			break;
		}
		return "UNKNOWN";
	}

}