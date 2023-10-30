#include "memory.hpp"

#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <mach/mach.h>

#include <vector>

bool vm_rpm_ptr(void *address, void *result, size_t len)
{
    if (!address)
        return false;

        // faster but will crash on any invalid address
#ifdef RPM_USE_MEMCPY
    return memcpy(result, address, len) != nullptr;
#else

    vm_size_t outSize = 0;
    kern_return_t kret = vm_read_overwrite(mach_task_self(), (vm_address_t)address,
                                           (vm_size_t)len, (vm_address_t)result, &outSize);

    return kret == 0 && outSize == len;

#endif
}

std::string vm_rpm_str(void *address, int max_len)
{
    std::vector<char> chars(max_len);
    if (!vm_rpm_ptr(address, chars.data(), max_len))
        return "";

    std::string str = "";
    for (size_t i = 0; i < chars.size(); i++)
    {
        if (chars[i] == '\0')
            break;
        str.push_back(chars[i]);
    }

    chars.clear();
    chars.shrink_to_fit();

    if ((int)str[0] == 0 && str.size() == 1)
        return "";

    return str;
}

uint8_t *GetSegmentData(const void *hdr, const char *seg, unsigned long *sz)
{
    if(!hdr || !seg) return nullptr;

#if defined(__LP64__)
    const mach_header_64 *header = (const mach_header_64 *)hdr;
#else
    const mach_header *header = (const mach_header *)hdr;
#endif

    return getsegmentdata(header, seg, sz);
}

namespace ioutils
{
    std::string remove_specials(std::string s)
    {
        for (size_t i = 0; i < s.size(); i++)
        {
            if (!((s[i] < 'A' || s[i] > 'Z') && (s[i] < 'a' || s[i] > 'z')))
                continue;

            if (!(s[i] < '0' || s[i] > '9'))
                continue;

            if (s[i] == '_')
                continue;

            s.erase(s.begin() + i);
            --i;
        }
        return s;
    }

    std::string replace_specials(std::string s, char c)
    {
        for (size_t i = 0; i < s.size(); i++)
        {
            if (!((s[i] < 'A' || s[i] > 'Z') && (s[i] < 'a' || s[i] > 'z')))
                continue;

            if (!(s[i] < '0' || s[i] > '9'))
                continue;

            if (s[i] == '_')
                continue;

            s[i] = c;
        }
        return s;
    }
}