#include "memory.hpp"

#include <vector>

bool vm_rpm_ptr(void *address, void *result, size_t len)
{
    if (address == NULL)
        return false;

        // faster but will crash on any invalid address
#ifdef RPM_USE_MEMCPY
    return memcpy(result, address, len) != NULL;
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
    for (int i = 0; i < chars.size(); i++)
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