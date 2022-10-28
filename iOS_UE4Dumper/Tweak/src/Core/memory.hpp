#pragma once

#include <unistd.h>
#include <cstdint>
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <mach/mach.h>
#include <string>

#include <types.h>

#define SET_FUNC_PTR(pfunc, addr) *reinterpret_cast<void **>(&pfunc) = (void *)(addr);

bool vm_rpm_ptr(void *address, void *result, size_t len);

template <typename T>
T vm_rpm_ptr(void *address)
{
    T buffer{};
    vm_rpm_ptr(address, &buffer, sizeof(T));
    return buffer;
}

std::string vm_rpm_str(void *address, int max_len = 0xff);

uint8_t *GetSegmentData(const void*, const char*, unsigned long*);