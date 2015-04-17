/*
 * Vulkan
 *
 * Copyright (C) 2014 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *   Chia-I Wu <olv@lunarg.com>
 */

#include "dev.h"
#include "mem.h"

VkResult intel_mem_alloc(struct intel_dev *dev,
                           const VkMemoryAllocInfo *info,
                           struct intel_mem **mem_ret)
{
    struct intel_mem *mem;

    /* ignore any IMAGE_INFO and BUFFER_INFO usage: they don't alter allocations */

    mem = (struct intel_mem *) intel_base_create(&dev->base.handle,
            sizeof(*mem), dev->base.dbg, VK_DBG_OBJECT_GPU_MEMORY, info, 0);
    if (!mem)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    mem->bo = intel_winsys_alloc_bo(dev->winsys,
            "vk-gpu-memory", info->allocationSize, 0);
    if (!mem->bo) {
        intel_mem_free(mem);
        return VK_ERROR_UNKNOWN;
    }

    mem->size = info->allocationSize;

    *mem_ret = mem;

    return VK_SUCCESS;
}

void intel_mem_free(struct intel_mem *mem)
{
    intel_bo_unref(mem->bo);

    intel_base_destroy(&mem->base);
}

VkResult intel_mem_import_userptr(struct intel_dev *dev,
                                    const void *userptr,
                                    size_t size,
                                    struct intel_mem **mem_ret)
{
    const size_t alignment = 4096;
    struct intel_mem *mem;

    if ((uintptr_t) userptr % alignment || size % alignment)
        return VK_ERROR_INVALID_ALIGNMENT;

    mem = (struct intel_mem *) intel_base_create(&dev->base.handle,
            sizeof(*mem), dev->base.dbg, VK_DBG_OBJECT_GPU_MEMORY, NULL, 0);
    if (!mem)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    mem->bo = intel_winsys_import_userptr(dev->winsys,
            "vk-gpu-memory-userptr", (void *) userptr, size, 0);
    if (!mem->bo) {
        intel_mem_free(mem);
        return VK_ERROR_UNKNOWN;
    }

    mem->size = size;

    *mem_ret = mem;

    return VK_SUCCESS;
}

VkResult intel_mem_set_priority(struct intel_mem *mem,
                                  VkMemoryPriority priority)
{
    /* pin the bo when VK_MEMORY_PRIORITY_VERY_HIGH? */
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkAllocMemory(
    VkDevice                                  device,
    const VkMemoryAllocInfo*                pAllocInfo,
    VkDeviceMemory*                             pMem)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_mem_alloc(dev, pAllocInfo, (struct intel_mem **) pMem);
}

ICD_EXPORT VkResult VKAPI vkFreeMemory(
    VkDeviceMemory                              mem_)
{
    struct intel_mem *mem = intel_mem(mem_);

    intel_mem_free(mem);

    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkSetMemoryPriority(
    VkDeviceMemory                              mem_,
    VkMemoryPriority                         priority)
{
    struct intel_mem *mem = intel_mem(mem_);

    return intel_mem_set_priority(mem, priority);
}

ICD_EXPORT VkResult VKAPI vkMapMemory(
    VkDeviceMemory                            mem_,
    VkDeviceSize                              offset,
    VkDeviceSize                              size,
    VkFlags                                   flags,
    void**                                    ppData)
{
    struct intel_mem *mem = intel_mem(mem_);
    void *ptr = intel_mem_map(mem, flags);

    *ppData = (void *)((size_t)ptr + offset);

    return (ptr) ? VK_SUCCESS : VK_ERROR_UNKNOWN;
}

ICD_EXPORT VkResult VKAPI vkUnmapMemory(
    VkDeviceMemory                              mem_)
{
    struct intel_mem *mem = intel_mem(mem_);

    intel_mem_unmap(mem);

    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkPinSystemMemory(
    VkDevice                                  device,
    const void*                                 pSysMem,
    size_t                                      memSize,
    VkDeviceMemory*                             pMem)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_mem_import_userptr(dev, pSysMem, memSize,
            (struct intel_mem **) pMem);
}

ICD_EXPORT VkResult VKAPI vkOpenSharedMemory(
    VkDevice                                  device,
    const VkMemoryOpenInfo*                 pOpenInfo,
    VkDeviceMemory*                             pMem)
{
    return VK_ERROR_UNAVAILABLE;
}

ICD_EXPORT VkResult VKAPI vkOpenPeerMemory(
    VkDevice                                  device,
    const VkPeerMemoryOpenInfo*            pOpenInfo,
    VkDeviceMemory*                             pMem)
{
    return VK_ERROR_UNAVAILABLE;
}
