// Export the VMA functions (NAPAPI) so that modules creating buffers and
// images through RenderService::getVulkanAllocator() can link against them
#include <utility/dllexport.h>
#define VMA_CALL_PRE NAPAPI
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

