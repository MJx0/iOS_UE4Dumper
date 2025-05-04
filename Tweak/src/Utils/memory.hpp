#pragma once

#include <unistd.h>
#include <cstdint>
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <mach/mach.h>
#include <string>
#include <vector>

#define SET_FUNC_PTR(pfunc, addr) *reinterpret_cast<void **>(&pfunc) = (void *)(addr);

bool vm_rpm_ptr(void *address, void *result, size_t len);

template <typename T>
T vm_rpm_ptr(void *address)
{
    T buffer{};
    vm_rpm_ptr(address, &buffer, sizeof(T));
    return buffer;
}

std::string vm_rpm_str(void *address, size_t max_len = 1024);
std::wstring vm_rpm_strw(void *address, size_t max_len = 1024);

namespace ioutils
{
    std::string remove_specials(std::string s);
    std::string replace_specials(std::string s, char c);
}
