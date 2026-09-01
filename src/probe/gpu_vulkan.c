/*
 * The GPU backend on Vulkan compute.
 *
 * Covers the host (llvmpipe in the build VM - software, for proving the pipeline) and the
 * Steam Deck's native build (radv on gfx1033 - real RDNA2). The code is identical; the
 * device underneath is the whole difference, which is why `obs_gpu_device_name` is carried
 * onto every result. Standard public Vulkan - nothing here is vendor-specific.
 *
 * # Init is cached, work is per-run
 *
 * The instance, device, queue and command pool are brought up once and kept. Each
 * `obs_gpu_run` creates and destroys only what is specific to that dispatch - the buffer,
 * the shader module, the pipeline - so a thousand kernels do not re-enumerate the device a
 * thousand times.
 *
 * # Only the host/native build compiles this
 *
 * It is in the native source lists, never the module's - the console gets `gpu_gnm.c`. So
 * `<vulkan/vulkan.h>` and `-lvulkan` never touch the freestanding build.
 */

#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include "obscene/gpu.h"

/* Tri-state so a failed bring-up is remembered rather than retried on every call. */
enum init_state { UNTRIED, READY, FAILED };
static enum init_state g_state = UNTRIED;

static VkInstance g_instance;
static VkPhysicalDevice g_phys;
static VkDevice g_device;
static VkQueue g_queue;
static uint32_t g_queue_family;
static VkCommandPool g_pool;
static char g_device_name[256] = "none";
static const char *g_device_type = "none";

const char *obs_gpu_backend_name(void) {
    return "vulkan";
}

const char *obs_gpu_device_name(void) {
    return g_device_name;
}

const char *obs_gpu_device_type(void) {
    return g_device_type;
}

/* The device type as a stable word, so provenance is gradable without parsing a name.
 * `cpu` is the one a consumer rejects for a hardware claim; the Deck's APU is `integrated`. */
static const char *device_type_name(VkPhysicalDeviceType type) {
    switch (type) {
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return "integrated";
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return "discrete";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        return "virtual";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return "cpu";
    default:
        return "other";
    }
}

/* Finds a host-visible, host-coherent memory type the buffer accepts.
 *
 * Coherent so no explicit flush is needed around the map - a probe values simplicity over
 * the marginal speed of non-coherent memory. Returns the index, or UINT32_MAX. */
static uint32_t find_host_memory(uint32_t type_bits) {
    VkPhysicalDeviceMemoryProperties mp;
    vkGetPhysicalDeviceMemoryProperties(g_phys, &mp);
    const VkMemoryPropertyFlags want =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++) {
        if ((type_bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & want) == want) {
            return i;
        }
    }
    return UINT32_MAX;
}

/* Brings up instance, device, queue and command pool once. Returns 1 on success. */
static int ensure_init(void) {
    if (g_state != UNTRIED) {
        return g_state == READY;
    }
    g_state = FAILED; /* until proven otherwise, so a partial failure is not retried */

    VkApplicationInfo app = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                             .apiVersion = VK_API_VERSION_1_1};
    VkInstanceCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                .pApplicationInfo = &app};
    if (vkCreateInstance(&ici, NULL, &g_instance) != VK_SUCCESS) {
        return 0;
    }

    uint32_t ndev = 0;
    vkEnumeratePhysicalDevices(g_instance, &ndev, NULL);
    if (ndev == 0) {
        return 0;
    }
    /* Prefer real silicon over a software rasteriser.
     *
     * A Steam Deck with the Mesa software driver also installed enumerates both its APU and
     * llvmpipe. Taking the first device would sometimes pick llvmpipe and quietly report CPU
     * results for a run meant to measure gfx1033 - the exact provenance failure the whole
     * GPU effort guards against. So the first non-CPU device wins, and a machine that only
     * has llvmpipe (the build VM) still falls back to it. The device *type* is recorded
     * alongside the name so a consumer can reject a `cpu` result without string-sniffing. */
    VkPhysicalDevice devs[8];
    uint32_t take = ndev < 8 ? ndev : 8;
    if (vkEnumeratePhysicalDevices(g_instance, &take, devs) != VK_SUCCESS || take == 0) {
        return 0;
    }
    g_phys = devs[0];
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(g_phys, &props);
    for (uint32_t i = 0; i < take; i++) {
        VkPhysicalDeviceProperties p;
        vkGetPhysicalDeviceProperties(devs[i], &p);
        if (p.deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU) {
            g_phys = devs[i];
            props = p;
            break;
        }
    }
    snprintf(g_device_name, sizeof(g_device_name), "%s", props.deviceName);
    g_device_type = device_type_name(props.deviceType);

    uint32_t nqf = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(g_phys, &nqf, NULL);
    if (nqf == 0) {
        return 0;
    }
    VkQueueFamilyProperties qf[32];
    if (nqf > 32) {
        nqf = 32;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(g_phys, &nqf, qf);
    uint32_t family = UINT32_MAX;
    for (uint32_t i = 0; i < nqf; i++) {
        if (qf[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            family = i;
            break;
        }
    }
    if (family == UINT32_MAX) {
        return 0;
    }
    g_queue_family = family;

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                   .queueFamilyIndex = family,
                                   .queueCount = 1,
                                   .pQueuePriorities = &prio};
    VkDeviceCreateInfo dci = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                              .queueCreateInfoCount = 1,
                              .pQueueCreateInfos = &qci};
    if (vkCreateDevice(g_phys, &dci, NULL, &g_device) != VK_SUCCESS) {
        return 0;
    }
    vkGetDeviceQueue(g_device, family, 0, &g_queue);

    VkCommandPoolCreateInfo cpci = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                    .queueFamilyIndex = family};
    if (vkCreateCommandPool(g_device, &cpci, NULL, &g_pool) != VK_SUCCESS) {
        return 0;
    }

    g_state = READY;
    return 1;
}

int obs_gpu_backend_available(void) {
    return ensure_init();
}

int obs_gpu_run(const uint32_t *spirv, size_t spirv_bytes, uint32_t *data,
                unsigned int count) {
    if (!ensure_init() || count == 0) {
        return -1;
    }

    const VkDeviceSize bytes = (VkDeviceSize)count * sizeof(uint32_t);
    int result = -1;

    /* Every handle initialised to null so the single cleanup path can destroy only what
     * was created, whichever step failed. */
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkShaderModule shader = VK_NULL_HANDLE;
    VkDescriptorSetLayout set_layout = VK_NULL_HANDLE;
    VkPipelineLayout pipe_layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkDescriptorPool desc_pool = VK_NULL_HANDLE;
    VkCommandBuffer cmd = VK_NULL_HANDLE;

    VkBufferCreateInfo bci = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                              .size = bytes,
                              .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                              .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
    if (vkCreateBuffer(g_device, &bci, NULL, &buffer) != VK_SUCCESS) {
        goto done;
    }

    VkMemoryRequirements mr;
    vkGetBufferMemoryRequirements(g_device, buffer, &mr);
    uint32_t mem_type = find_host_memory(mr.memoryTypeBits);
    if (mem_type == UINT32_MAX) {
        goto done;
    }
    VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                .allocationSize = mr.size,
                                .memoryTypeIndex = mem_type};
    if (vkAllocateMemory(g_device, &mai, NULL, &memory) != VK_SUCCESS
        || vkBindBufferMemory(g_device, buffer, memory, 0) != VK_SUCCESS) {
        goto done;
    }

    /* Upload the inputs. */
    void *mapped = NULL;
    if (vkMapMemory(g_device, memory, 0, bytes, 0, &mapped) != VK_SUCCESS) {
        goto done;
    }
    memcpy(mapped, data, (size_t)bytes);
    vkUnmapMemory(g_device, memory);

    VkShaderModuleCreateInfo smci = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                     .codeSize = spirv_bytes,
                                     .pCode = spirv};
    if (vkCreateShaderModule(g_device, &smci, NULL, &shader) != VK_SUCCESS) {
        goto done;
    }

    VkDescriptorSetLayoutBinding binding = {.binding = 0,
                                            .descriptorType =
                                                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            .descriptorCount = 1,
                                            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT};
    VkDescriptorSetLayoutCreateInfo dslci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &binding};
    if (vkCreateDescriptorSetLayout(g_device, &dslci, NULL, &set_layout) != VK_SUCCESS) {
        goto done;
    }
    VkPipelineLayoutCreateInfo plci = {.sType =
                                           VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                       .setLayoutCount = 1,
                                       .pSetLayouts = &set_layout};
    if (vkCreatePipelineLayout(g_device, &plci, NULL, &pipe_layout) != VK_SUCCESS) {
        goto done;
    }

    VkPipelineShaderStageCreateInfo stage = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shader,
        .pName = "main"};
    VkComputePipelineCreateInfo cpci = {.sType =
                                            VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
                                        .stage = stage,
                                        .layout = pipe_layout};
    if (vkCreateComputePipelines(g_device, VK_NULL_HANDLE, 1, &cpci, NULL, &pipeline)
        != VK_SUCCESS) {
        goto done;
    }

    VkDescriptorPoolSize pool_size = {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                      .descriptorCount = 1};
    VkDescriptorPoolCreateInfo dpci = {.sType =
                                           VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                       .maxSets = 1,
                                       .poolSizeCount = 1,
                                       .pPoolSizes = &pool_size};
    if (vkCreateDescriptorPool(g_device, &dpci, NULL, &desc_pool) != VK_SUCCESS) {
        goto done;
    }
    VkDescriptorSetAllocateInfo dsai = {.sType =
                                            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                        .descriptorPool = desc_pool,
                                        .descriptorSetCount = 1,
                                        .pSetLayouts = &set_layout};
    VkDescriptorSet set = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(g_device, &dsai, &set) != VK_SUCCESS) {
        goto done;
    }
    VkDescriptorBufferInfo dbi = {.buffer = buffer, .offset = 0, .range = bytes};
    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                  .dstSet = set,
                                  .dstBinding = 0,
                                  .descriptorCount = 1,
                                  .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                  .pBufferInfo = &dbi};
    vkUpdateDescriptorSets(g_device, 1, &write, 0, NULL);

    VkCommandBufferAllocateInfo cbai = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = g_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1};
    if (vkAllocateCommandBuffers(g_device, &cbai, &cmd) != VK_SUCCESS) {
        goto done;
    }
    VkCommandBufferBeginInfo cbbi = {.sType =
                                         VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                     .flags =
                                         VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    if (vkBeginCommandBuffer(cmd, &cbbi) != VK_SUCCESS) {
        goto done;
    }
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipe_layout, 0, 1, &set, 0,
                            NULL);
    /* One workgroup per 64 lanes, rounded up. The shader bounds-checks the tail. */
    vkCmdDispatch(cmd, (count + 63u) / 64u, 1, 1);
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        goto done;
    }

    VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                       .commandBufferCount = 1,
                       .pCommandBuffers = &cmd};
    if (vkQueueSubmit(g_queue, 1, &si, VK_NULL_HANDLE) != VK_SUCCESS
        || vkQueueWaitIdle(g_queue) != VK_SUCCESS) {
        goto done;
    }

    /* Read the results back over the inputs. */
    if (vkMapMemory(g_device, memory, 0, bytes, 0, &mapped) != VK_SUCCESS) {
        goto done;
    }
    memcpy(data, mapped, (size_t)bytes);
    vkUnmapMemory(g_device, memory);
    result = 0;

done:
    if (cmd != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(g_device, g_pool, 1, &cmd);
    }
    if (desc_pool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(g_device, desc_pool, NULL);
    }
    if (pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(g_device, pipeline, NULL);
    }
    if (pipe_layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(g_device, pipe_layout, NULL);
    }
    if (set_layout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(g_device, set_layout, NULL);
    }
    if (shader != VK_NULL_HANDLE) {
        vkDestroyShaderModule(g_device, shader, NULL);
    }
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(g_device, memory, NULL);
    }
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(g_device, buffer, NULL);
    }
    return result;
}
