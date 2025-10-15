#pragma once
#include <vulkan/vulkan.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#define VK_CHECK(x)                                                      \
    do {                                                                 \
        VkResult _err = (x);                                             \
        if (_err < 0) {                  \
            fprintf(stderr, "Vulkan ERROR (%d) at %s:%d\n",              \
                    (int)_err, __FILE__, __LINE__);                      \
            abort();                                                     \
        }\
        else if(_err > 0) { \
           fprintf(stderr, "Vulkan ERROR (%d) at %s:%d\n",              \
                    (int)_err, __FILE__, __LINE__);   \
        }\
    } while (0)