#include "UEPackage.hpp"

#include <fmt/core.h>

#include "memory.hpp"

void UE_UPackage::GenerateFunction(UE_UFunction fn, Function *out)
{
	out->Name = fn.GetName();
	out->FullName = fn.GetFullName();
	out->Flags = fn.GetFunctionFlags();
	out->NumParams = fn.GetNumParams();
	out->ParamSize = fn.GetParamSize();
	out->ReturnValueOffset = fn.GetReturnValueOffset();
	out->Func = fn.GetFunc();

	auto generateParam = [&](IProperty *prop)
	{
		auto flags = prop->GetPropertyFlags();

		// if property has 'ReturnParm' flag
		if (flags & 0x400)
		{
			out->CppName = prop->GetType().second + " " + fn.GetName();
		}
		// if property has 'Parm' flag
		else if (flags & 0x80)
		{
			if (prop->GetArrayDim() > 1)
			{
				out->Params += fmt::format("{}* {}, ", prop->GetType().second, prop->GetName());
			}
			else
			{
				if (flags & 0x100)
				{
					out->Params += fmt::format("{}& {}, ", prop->GetType().second, prop->GetName());
				}
				else
				{
					out->Params += fmt::format("{} {}, ", prop->GetType().second, prop->GetName());
				}
			}
		}
	};

	for (auto prop = fn.GetChildProperties().Cast<UE_FProperty>(); prop; prop = prop.GetNext().Cast<UE_FProperty>())
	{
		auto propInterface = prop.GetInterface();
		generateParam(&propInterface);
	}
	for (auto prop = fn.GetChildren().Cast<UE_UProperty>(); prop; prop = prop.GetNext().Cast<UE_UProperty>())
	{
		auto propInterface = prop.GetInterface();
		generateParam(&propInterface);
	}
	if (out->Params.size())
	{
		out->Params.erase(out->Params.size() - 2);
	}

	if (out->CppName.size() == 0)
	{
		out->CppName = "void " + fn.GetName();
	}
}

void UE_UPackage::GenerateStruct(UE_UStruct object, std::vector<Struct> &arr)
{
	Struct s;
	s.Size = object.GetSize();
	if (s.Size == 0)
	{
		return;
	}
	s.Inherited = 0;
	s.Name = object.GetName();
	s.FullName = object.GetFullName();
	s.CppName = "struct " + object.GetCppName();

	auto super = object.GetSuper();
	if (super)
	{
		s.CppName += " : " + super.GetCppName();
		s.Inherited = super.GetSize();
	}

	uint32 offset = s.Inherited;
	uint8 bitOffset = 0;

	auto generateMember = [&](IProperty *prop, Member *m)
	{
		auto arrDim = prop->GetArrayDim();
		m->Size = prop->GetSize() * arrDim;
		if (m->Size == 0)
		{
			return;
		} // this shouldn't be zero

		auto type = prop->GetType();
		m->Type = type.second;
		m->Name = prop->GetName();
		m->Offset = prop->GetOffset();

		if (m->Offset > offset)
		{
			UE_UPackage::FillPadding(object, s.Members, offset, bitOffset, m->Offset);
		}
		if (type.first == PropertyType::BoolProperty && *(uint32 *)type.second.data() != 'loob')
		{
			auto boolProp = prop;
			auto mask = boolProp->GetFieldMask();
			uint8 zeros = 0, ones = 0;
			while (mask & ~1)
			{
				mask >>= 1;
				zeros++;
			}
			while (mask & 1)
			{
				mask >>= 1;
				ones++;
			}
			if (zeros > bitOffset)
			{
				UE_UPackage::GenerateBitPadding(s.Members, offset, bitOffset, zeros - bitOffset);
				bitOffset = zeros;
			}
			m->Name += fmt::format(" : {}", ones);
			bitOffset += ones;

			if (bitOffset == 8)
			{
				offset++;
				bitOffset = 0;
			}
		}
		else
		{
			if (arrDim > 1)
			{
				m->Name += fmt::format("[{:#0x}]", arrDim);
			}

			offset += m->Size;
		}
	};

	for (auto prop = object.GetChildProperties().Cast<UE_FProperty>(); prop; prop = prop.GetNext().Cast<UE_FProperty>())
	{
		Member m;
		auto propInterface = prop.GetInterface();
		generateMember(&propInterface, &m);
		s.Members.push_back(m);
	}

	for (auto child = object.GetChildren(); child; child = child.GetNext())
	{
		if (child.IsA<UE_UFunction>())
		{
			auto fn = child.Cast<UE_UFunction>();
			Function f;
			GenerateFunction(fn, &f);
			s.Functions.push_back(f);
		}
		else if (child.IsA<UE_UProperty>())
		{
			auto prop = child.Cast<UE_UProperty>();
			Member m;
			auto propInterface = prop.GetInterface();
			generateMember(&propInterface, &m);
			s.Members.push_back(m);
		}
	}

	if (s.Size > offset)
	{
		UE_UPackage::FillPadding(object, s.Members, offset, bitOffset, s.Size);
	}

	arr.push_back(s);
}

void UE_UPackage::GenerateEnum(UE_UEnum object, std::vector<Enum> &arr)
{
	Enum e;
	e.FullName = object.GetFullName();

	auto names = object.GetNames();

	uint64 max = 0;
	uint64 nameSize = ((Profile::offsets.FName.Number + 4) + 7) & ~(7);
	uint64 pairSize = nameSize + 8;

	for (uint32 i = 0; i < names.Count; i++)
	{
		auto pair = names.Data + i * pairSize;
		auto name = UE_FName(pair);
		auto str = name.GetName();
		auto pos = str.find_last_of(':');
		if (pos != std::string::npos)
		{
			str = str.substr(pos + 1);
		}

		auto value = vm_rpm_ptr<int64>(pair + nameSize);
		if ((uint64)value > max)
			max = value;

		str.append(" = ").append(fmt::format("{}", value));
		e.Members.push_back(str);
	}

	const char *type = nullptr;

	// I didn't see int16 yet, so I assume the engine generates only int32 and uint8:
	if (max > 256)
	{
		type = " : int32"; // I assume if enum has a negative value it is int32
	}
	else
	{
		type = " : uint8";
	}

	e.CppName = "enum class " + object.GetName() + type;

	if (e.Members.size())
	{
		arr.push_back(e);
	}
}

void UE_UPackage::GenerateBitPadding(std::vector<Member> &members, uint32 offset, uint8 bitOffset, uint8 size)
{
	Member padding;
	padding.Type = "char";
	padding.Name = fmt::format("pad_0x{:0X}_{} : {}", offset, bitOffset, size);
	padding.Offset = offset;
	padding.Size = 1;
	members.push_back(padding);
}

void UE_UPackage::GeneratePadding(std::vector<Member> &members, uint32 offset, uint32 size)
{
	Member padding;
	padding.Type = "char";
	padding.Name = fmt::format("pad_0x{:0X}[{:#0x}]", offset, size);
	padding.Offset = offset;
	padding.Size = size;
	members.push_back(padding);
}

void UE_UPackage::FillPadding(UE_UStruct object, std::vector<Member> &members, uint32 &offset, uint8 &bitOffset, uint32 end)
{
	if (bitOffset && bitOffset < 8)
	{
		UE_UPackage::GenerateBitPadding(members, offset, bitOffset, 8 - bitOffset);
		bitOffset = 0;
		offset++;
	}

	if (offset != end)
	{
		GeneratePadding(members, offset, end - offset);
		offset = end;
	}
}

void UE_UPackage::SaveStruct(std::vector<Struct> &arr, FILE *file)
{
	for (auto &s : arr)
	{
		fmt::print(file, "// Object Name: {}\n// Size: {:#04x} // Inherited bytes: {:#04x}\n{} {{", s.FullName, s.Size, s.Inherited, s.CppName);

		if (s.Members.size())
		{
			fmt::print(file, "\n\t// Fields");
			for (auto &m : s.Members)
			{
				fmt::print(file, "\n\t{} {}; // Offset: {:#04x} // Size: {:#04x}", m.Type, m.Name, m.Offset, m.Size);
			}
		}
		if (s.Functions.size())
		{
			fmt::print(file, "{}\n\t// Functions", s.Members.size() ? "\n" : "");
			for (auto &f : s.Functions)
			{
				fmt::print(file, "\n\n\t// Object Name: {}\n\t// Flags: [{}]\n\t{}({}); // Offset: {:#08x} // Return & Params: Num({}) Size({:#0x})", f.FullName, f.Flags, f.CppName, f.Params, f.Func - Profile::BaseAddress, f.NumParams, f.ParamSize);
			}
		}
		fmt::print(file, "\n}};\n\n");
	}
}

void UE_UPackage::SaveEnum(std::vector<Enum> &arr, FILE *file)
{
	for (auto &e : arr)
	{
		fmt::print(file, "// Object Name: {}\n{} {{", e.FullName, e.CppName);

		auto lastIdx = e.Members.size() - 1;
		for (auto i = 0; i < lastIdx; i++)
		{
			auto &m = e.Members.at(i);
			fmt::print(file, "\n\t{},", m);
		}

		auto &m = e.Members.at(lastIdx);
		fmt::print(file, "\n\t{}", m);

		fmt::print(file, "\n}};\n\n");
	}
}

void UE_UPackage::Process()
{
	auto &objects = Package->second;
	for (auto &object : objects)
	{
		if (object.IsA<UE_UClass>())
		{
			GenerateStruct(object.Cast<UE_UStruct>(), Classes);
		}
		else if (object.IsA<UE_UScriptStruct>())
		{
			GenerateStruct(object.Cast<UE_UStruct>(), Structures);
		}
		else if (object.IsA<UE_UEnum>())
		{
			GenerateEnum(object.Cast<UE_UEnum>(), Enums);
		}
	}
}

bool UE_UPackage::Save(FILE *fulldump_file, const char *dump_dir)
{
	if (!fulldump_file && dump_dir == NULL)
		return false;

	if (!(Classes.size() || Structures.size() || Enums.size()))
	{
		return false;
	}

	std::string packageName = GetObject().GetName();
	char chars[] = "/\\:*?\"<>|";
	for (auto c : chars)
	{
		auto pos = packageName.find(c);
		if (pos != std::string::npos)
		{
			packageName[pos] = '_';
		}
	}

	if (fulldump_file)
	{
		fmt::print(fulldump_file, "// {} Dumping: [ Enums: {} | Structs: {} | Classes: {} ]\n\n", packageName, Enums.size(), Structures.size(), Classes.size());
	}

	if (Enums.size())
	{
		if (dump_dir != NULL)
		{
			std::string file_path = dump_dir;
			file_path += "/";
			file_path += packageName;
			file_path += "_enums.hpp";
			File file(file_path.c_str(), "w");
			if (!file)
				return false;

			fmt::print(file, "#pragma once\n\n#include <cstdio>\n#include <string>\n#include <cstdint>\n\n");
			UE_UPackage::SaveEnum(Enums, file);
		}

		if (fulldump_file)
		{
			UE_UPackage::SaveEnum(Enums, fulldump_file);
			fmt::print(fulldump_file, "\n\n");
		}
	}

	if (Structures.size())
	{
		if (dump_dir != NULL)
		{
			std::string file_path = dump_dir;
			file_path += "/";
			file_path += packageName;
			file_path += "_structs.hpp";
			File file(file_path.c_str(), "w");
			if (!file)
				return false;

			fmt::print(file, "#pragma once\n\n#include <cstdio>\n#include <string>\n#include <cstdint>\n\n");
			UE_UPackage::SaveStruct(Structures, file);
		}

		if (fulldump_file)
		{
			UE_UPackage::SaveStruct(Structures, fulldump_file);
			fmt::print(fulldump_file, "\n\n");
		}
	}

	if (Classes.size())
	{
		if (dump_dir != NULL)
		{
			std::string file_path = dump_dir;
			file_path += "/";
			file_path += packageName;
			file_path += "_classes.hpp";
			File file(file_path.c_str(), "w");
			if (!file)
				return false;

			fmt::print(file, "#pragma once\n\n#include <cstdio>\n#include <string>\n#include <cstdint>\n\n");
			UE_UPackage::SaveStruct(Classes, file);
		}

		if (fulldump_file)
		{
			UE_UPackage::SaveStruct(Classes, fulldump_file);
			fmt::print(fulldump_file, "\n\n");
		}
	}

	if (fulldump_file)
	{
		fmt::print(fulldump_file, "\n\n");
	}

	return true;
}

UE_UObject UE_UPackage::GetObject() const { return UE_UObject(Package->first); }