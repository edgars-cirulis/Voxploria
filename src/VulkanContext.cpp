#include "VulkanContext.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
#include "Frustum.hpp"
#include "Types.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifndef SHADER_DIR
#define SHADER_DIR "."
#endif

static bool fileExists(const std::string& p)
{
    std::ifstream f(p, std::ios::binary);
    return (bool)f;
}
static std::string findFirstExisting(const std::vector<std::string>& c)
{
    for (const auto& p : c)
        if (fileExists(p))
            return p;
    return {};
}
static std::string shaderPath(const char* name)
{
    auto p =
        findFirstExisting({ std::string(SHADER_DIR) + "/" + name, std::string("./shaders/") + name, std::string("../shaders/") + name });
    if (p.empty())
        throw std::runtime_error(std::string("Shader not found: ") + name);
    return p;
}
static std::string assetPath(const char* name)
{
    auto p = findFirstExisting({ std::string("./assets/") + name, std::string("../assets/") + name, std::string(name) });
    if (p.empty())
        throw std::runtime_error(std::string("Asset not found: ") + name);
    return p;
}
static std::vector<char> readFile(const std::string& p)
{
    std::ifstream f(p, std::ios::binary);
    if (!f)
        throw std::runtime_error("readFile: " + p);
    f.seekg(0, std::ios::end);
    size_t sz = f.tellg();
    f.seekg(0);
    std::vector<char> d(sz);
    f.read(d.data(), sz);
    return d;
}

uint32_t VulkanContext::findMemType(uint32_t typeBits, VkMemoryPropertyFlags flags)
{
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeBits & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & flags) == flags)
            return i;
    }
    throw std::runtime_error("No memory type");
}

void VulkanContext::createBuffer(VkDeviceSize size,
                                 VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags props,
                                 VkBuffer& buf,
                                 VkDeviceMemory& mem)
{
    VkBufferCreateInfo bci{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bci.size = size;
    bci.usage = usage;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &bci, nullptr, &buf) != VK_SUCCESS)
        throw std::runtime_error("buffer");
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(device, buf, &req);
    VkMemoryAllocateInfo mai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    mai.allocationSize = req.size;
    mai.memoryTypeIndex = findMemType(req.memoryTypeBits, props);
    if (vkAllocateMemory(device, &mai, nullptr, &mem) != VK_SUCCESS)
        throw std::runtime_error("alloc");
    vkBindBufferMemory(device, buf, mem, 0);
}

void VulkanContext::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo a{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    a.commandPool = cmdPool;
    a.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    a.commandBufferCount = 1;
    VkCommandBuffer cb;
    vkAllocateCommandBuffers(device, &a, &cb);

    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &bi);

    VkBufferCopy reg{ 0, 0, size };
    vkCmdCopyBuffer(cb, src, dst, 1, &reg);

    vkEndCommandBuffer(cb);
    VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cb;
    vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, cmdPool, 1, &cb);
}

void VulkanContext::createImage(uint32_t w,
                                uint32_t h,
                                VkFormat fmt,
                                VkImageTiling tiling,
                                VkImageUsageFlags usage,
                                VkMemoryPropertyFlags props,
                                VkImage& img,
                                VkDeviceMemory& mem)
{
    VkImageCreateInfo ici{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.extent = { w, h, 1 };
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    ici.format = fmt;
    ici.tiling = tiling;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ici.usage = usage;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &ici, nullptr, &img) != VK_SUCCESS)
        throw std::runtime_error("image");
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(device, img, &req);
    VkMemoryAllocateInfo mai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    mai.allocationSize = req.size;
    mai.memoryTypeIndex = findMemType(req.memoryTypeBits, props);
    if (vkAllocateMemory(device, &mai, nullptr, &mem) != VK_SUCCESS)
        throw std::runtime_error("img mem");
    vkBindImageMemory(device, img, mem, 0);
}

VkImageView VulkanContext::createImageView(VkImage img, VkFormat fmt)
{
    VkImageViewCreateInfo iv{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    iv.image = img;
    iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv.format = fmt;
    iv.subresourceRange.aspectMask =
        (fmt == VK_FORMAT_D32_SFLOAT || fmt == VK_FORMAT_D24_UNORM_S8_UINT || fmt == VK_FORMAT_D32_SFLOAT_S8_UINT)
            ? VK_IMAGE_ASPECT_DEPTH_BIT
            : VK_IMAGE_ASPECT_COLOR_BIT;
    iv.subresourceRange.levelCount = 1;
    iv.subresourceRange.layerCount = 1;
    VkImageView view{};
    if (vkCreateImageView(device, &iv, nullptr, &view) != VK_SUCCESS)
        throw std::runtime_error("img view");
    return view;
}

void VulkanContext::transitionImageLayout(VkImage img, VkFormat, VkImageLayout oldL, VkImageLayout newL)
{
    VkCommandBufferAllocateInfo a{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    a.commandPool = cmdPool;
    a.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    a.commandBufferCount = 1;
    VkCommandBuffer cb;
    vkAllocateCommandBuffers(device, &a, &cb);
    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(cb, &bi);

    VkImageMemoryBarrier b{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    b.oldLayout = oldL;
    b.newLayout = newL;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = img;
    b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    b.subresourceRange.levelCount = 1;
    b.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    b.srcAccessMask = 0;
    b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    if (oldL == VK_IMAGE_LAYOUT_UNDEFINED && newL == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        b.srcAccessMask = 0;
        b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (oldL == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newL == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }

    vkCmdPipelineBarrier(cb, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &b);
    vkEndCommandBuffer(cb);
    VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cb;
    vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, cmdPool, 1, &cb);
}

void VulkanContext::copyBufferToImage(VkBuffer buf, VkImage img, uint32_t w, uint32_t h)
{
    VkCommandBufferAllocateInfo a{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    a.commandPool = cmdPool;
    a.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    a.commandBufferCount = 1;
    VkCommandBuffer cb;
    vkAllocateCommandBuffers(device, &a, &cb);
    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(cb, &bi);
    VkBufferImageCopy reg{};
    reg.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    reg.imageSubresource.layerCount = 1;
    reg.imageExtent = { w, h, 1 };
    vkCmdCopyBufferToImage(cb, buf, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &reg);
    vkEndCommandBuffer(cb);
    VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cb;
    vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(device, cmdPool, 1, &cb);
}

static VkFormat pickDepth(VkPhysicalDevice phys)
{
    const VkFormat cand[] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };
    for (VkFormat f : cand) {
        VkFormatProperties p;
        vkGetPhysicalDeviceFormatProperties(phys, f, &p);
        if (p.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return f;
    }
    return VK_FORMAT_D32_SFLOAT;
}
VkFormat VulkanContext::findDepthFormat()
{
    return pickDepth(phys);
}

void VulkanContext::createDepthResources()
{
    VkFormat df = findDepthFormat();
    createImage(swapExtent.width, swapExtent.height, df, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthMem);
    VkImageViewCreateInfo iv{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    iv.image = depthImage;
    iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv.format = df;
    iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    iv.subresourceRange.levelCount = 1;
    iv.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &iv, nullptr, &depthView) != VK_SUCCESS)
        throw std::runtime_error("depth view");
}

void VulkanContext::destroyDepthResources()
{
    if (depthView) {
        vkDestroyImageView(device, depthView, nullptr);
        depthView = {};
    }
    if (depthImage) {
        vkDestroyImage(device, depthImage, nullptr);
        depthImage = {};
    }
    if (depthMem) {
        vkFreeMemory(device, depthMem, nullptr);
        depthMem = {};
    }
}

void VulkanContext::init(GLFWwindow* win, int, int)
{
    window = win;

    VkApplicationInfo app{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app.pApplicationName = "Voxploria";
    app.apiVersion = VK_API_VERSION_1_2;

    uint32_t extCount = 0;
    const char** exts = glfwGetRequiredInstanceExtensions(&extCount);
    VkInstanceCreateInfo ici{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    ici.pApplicationInfo = &app;
    ici.enabledExtensionCount = extCount;
    ici.ppEnabledExtensionNames = exts;
#ifndef NDEBUG
    const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
    ici.enabledLayerCount = 1;
    ici.ppEnabledLayerNames = layers;
#endif
    if (vkCreateInstance(&ici, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("vkCreateInstance");
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("surface");

    uint32_t pc = 0;
    vkEnumeratePhysicalDevices(instance, &pc, nullptr);
    std::vector<VkPhysicalDevice> devs(pc);
    vkEnumeratePhysicalDevices(instance, &pc, devs.data());
    for (auto d : devs) {
        uint32_t qc = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(d, &qc, nullptr);
        std::vector<VkQueueFamilyProperties> qfps(qc);
        vkGetPhysicalDeviceQueueFamilyProperties(d, &qc, qfps.data());
        for (uint32_t i = 0; i < qc; ++i) {
            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(d, i, surface, &present);
            if ((qfps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
                phys = d;
                queueFamily = i;
                break;
            }
        }
        if (phys)
            break;
    }
    if (!phys)
        throw std::runtime_error("no device");

    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    qci.queueFamilyIndex = queueFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;

    const char* devExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkPhysicalDeviceFeatures feats{};

    VkDeviceCreateInfo dci{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = devExts;
    dci.pEnabledFeatures = &feats;
    if (vkCreateDevice(phys, &dci, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("device");
    vkGetDeviceQueue(device, queueFamily, 0, &queue);
    vkGetPhysicalDeviceMemoryProperties(phys, &memProps);

    VkCommandPoolCreateInfo cp{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    cp.queueFamilyIndex = queueFamily;
    cp.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(device, &cp, nullptr, &cmdPool) != VK_SUCCESS)
        throw std::runtime_error("cmd pool");

    VkDescriptorSetLayoutBinding sampB{};
    sampB.binding = 0;
    sampB.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampB.descriptorCount = 1;
    sampB.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo slci{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    slci.bindingCount = 1;
    slci.pBindings = &sampB;
    if (vkCreateDescriptorSetLayout(device, &slci, nullptr, &setLayout) != VK_SUCCESS)
        throw std::runtime_error("setLayout");

    VkPushConstantRange pcr{};
    pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pcr.offset = 0;
    pcr.size = 96;

    VkPipelineLayoutCreateInfo plci{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    plci.setLayoutCount = 1;
    plci.pSetLayouts = &setLayout;
    plci.pushConstantRangeCount = 1;
    plci.pPushConstantRanges = &pcr;
    if (vkCreatePipelineLayout(device, &plci, nullptr, &pipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("pipelineLayout");

    VkDescriptorPoolSize dps{};
    dps.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dps.descriptorCount = 1;
    VkDescriptorPoolCreateInfo dpci{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    dpci.poolSizeCount = 1;
    dpci.pPoolSizes = &dps;
    dpci.maxSets = 1;
    if (vkCreateDescriptorPool(device, &dpci, nullptr, &descPool) != VK_SUCCESS)
        throw std::runtime_error("descPool");
    VkDescriptorSetAllocateInfo dsai{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    dsai.descriptorPool = descPool;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts = &setLayout;
    if (vkAllocateDescriptorSets(device, &dsai, &descSet) != VK_SUCCESS)
        throw std::runtime_error("descSet");

    VkSamplerCreateInfo samp{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samp.magFilter = VK_FILTER_NEAREST;
    samp.minFilter = VK_FILTER_NEAREST;
    samp.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samp.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samp.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samp.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samp.minLod = 0.0f;
    samp.maxLod = 0.0f;
    if (vkCreateSampler(device, &samp, nullptr, &sampler) != VK_SUCCESS)
        throw std::runtime_error("sampler");

    createSwapchain();
    allocCommandBuffersForSwapchain();
    createSyncObjectsForSwapchain();
    rebuildGraphicsPipeline();
}

void VulkanContext::rebuildGraphicsPipeline()
{
    auto vsp = shaderPath("voxel.vert.spv");
    auto fsp = shaderPath("voxel.frag.spv");
    auto vert = readFile(vsp);
    auto frag = readFile(fsp);

    VkShaderModuleCreateInfo smci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    smci.codeSize = vert.size();
    smci.pCode = reinterpret_cast<const uint32_t*>(vert.data());
    VkShaderModule vs;
    if (vkCreateShaderModule(device, &smci, nullptr, &vs) != VK_SUCCESS)
        throw std::runtime_error("vs");
    smci.codeSize = frag.size();
    smci.pCode = reinterpret_cast<const uint32_t*>(frag.data());
    VkShaderModule fs;
    if (vkCreateShaderModule(device, &smci, nullptr, &fs) != VK_SUCCESS)
        throw std::runtime_error("fs");

    VkVertexInputBindingDescription vb{};
    vb.binding = 0;
    vb.stride = sizeof(Vertex);
    vb.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 5> va{};
    va[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) };
    va[1] = { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) };
    va[2] = { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) };
    va[3] = { 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tile) };
    va[4] = { 4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) };

    VkPipelineVertexInputStateCreateInfo vi{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vi.vertexBindingDescriptionCount = 1;
    vi.pVertexBindingDescriptions = &vb;
    vi.vertexAttributeDescriptionCount = (uint32_t)va.size();
    vi.pVertexAttributeDescriptions = va.data();

    VkPipelineInputAssemblyStateCreateInfo ia{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vp{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    vp.viewportCount = 1;
    vp.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rs{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState cba{};
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo cb{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    cb.attachmentCount = 1;
    cb.pAttachments = &cba;

    VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dyn{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dyn.dynamicStateCount = 2;
    dyn.pDynamicStates = dynStates;

    if (pipeline)
        vkDestroyPipeline(device, pipeline, nullptr);

    VkPipelineShaderStageCreateInfo s0{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    s0.stage = VK_SHADER_STAGE_VERTEX_BIT;
    s0.module = vs;
    s0.pName = "main";
    VkPipelineShaderStageCreateInfo s1{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    s1.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    s1.module = fs;
    s1.pName = "main";
    VkPipelineShaderStageCreateInfo stages[2] = { s0, s1 };

    VkGraphicsPipelineCreateInfo gp{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    gp.stageCount = 2;
    gp.pStages = stages;
    gp.pVertexInputState = &vi;
    gp.pInputAssemblyState = &ia;
    gp.pViewportState = &vp;
    gp.pRasterizationState = &rs;
    gp.pMultisampleState = &ms;
    gp.pDepthStencilState = &ds;
    gp.pColorBlendState = &cb;
    gp.pDynamicState = &dyn;
    gp.layout = pipelineLayout;
    gp.renderPass = renderPass;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gp, nullptr, &pipeline) != VK_SUCCESS)
        throw std::runtime_error("pipeline");

    vkDestroyShaderModule(device, fs, nullptr);
    vkDestroyShaderModule(device, vs, nullptr);
}

void VulkanContext::allocCommandBuffersForSwapchain()
{
    if (!cmdBufs.empty()) {
        vkFreeCommandBuffers(device, cmdPool, (uint32_t)cmdBufs.size(), cmdBufs.data());
        cmdBufs.clear();
    }
    cmdBufs.resize((uint32_t)swapImages.size());
    VkCommandBufferAllocateInfo a{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    a.commandPool = cmdPool;
    a.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    a.commandBufferCount = (uint32_t)cmdBufs.size();
    vkAllocateCommandBuffers(device, &a, cmdBufs.data());
}

void VulkanContext::createSwapchain()
{
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, surface, &caps);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> fmts(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &formatCount, fmts.data());
    if (fmts.empty())
        throw std::runtime_error("No surface formats!");

    VkSurfaceFormatKHR chosenFmt = fmts[0];
    for (auto& f : fmts) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosenFmt = f;
            break;
        }
    }
    swapFormat = chosenFmt.format;

    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    while (fbw == 0 || fbh == 0) {
        glfwWaitEvents();
        glfwGetFramebufferSize(window, &fbw, &fbh);
    }
    if (caps.currentExtent.width != UINT32_MAX) {
        swapExtent = caps.currentExtent;
    } else {
        swapExtent.width = std::clamp<uint32_t>(fbw, caps.minImageExtent.width, caps.maxImageExtent.width);
        swapExtent.height = std::clamp<uint32_t>(fbh, caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    uint32_t pmCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &pmCount, nullptr);
    std::vector<VkPresentModeKHR> pms(pmCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &pmCount, pms.data());
    auto has = [&](VkPresentModeKHR m) { return std::find(pms.begin(), pms.end(), m) != pms.end(); };

    VkPresentModeKHR present = VK_PRESENT_MODE_FIFO_KHR;
    if (has(VK_PRESENT_MODE_MAILBOX_KHR))
        present = VK_PRESENT_MODE_MAILBOX_KHR;
    else if (has(VK_PRESENT_MODE_IMMEDIATE_KHR))
        present = VK_PRESENT_MODE_IMMEDIATE_KHR;

    VkCompositeAlphaFlagBitsKHR comp = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    const VkCompositeAlphaFlagBitsKHR pref[] = { VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                                                 VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR };
    for (auto b : pref) {
        if (caps.supportedCompositeAlpha & b) {
            comp = b;
            break;
        }
    }

    uint32_t imageCount = std::max(2u, caps.minImageCount);
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
        imageCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR sc{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    sc.surface = surface;
    sc.minImageCount = imageCount;
    sc.imageFormat = chosenFmt.format;
    sc.imageColorSpace = chosenFmt.colorSpace;
    sc.imageExtent = swapExtent;
    sc.imageArrayLayers = 1;
    sc.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sc.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sc.preTransform = caps.currentTransform;
    sc.compositeAlpha = comp;
    sc.presentMode = present;
    sc.clipped = VK_TRUE;
    sc.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &sc, nullptr, &swapchain) != VK_SUCCESS)
        throw std::runtime_error("vkCreateSwapchainKHR failed");

    uint32_t ic = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &ic, nullptr);
    swapImages.resize(ic);
    vkGetSwapchainImagesKHR(device, swapchain, &ic, swapImages.data());

    swapViews.resize(ic);
    for (uint32_t i = 0; i < ic; ++i)
        swapViews[i] = createImageView(swapImages[i], swapFormat);

    if (renderPass)
        vkDestroyRenderPass(device, renderPass, nullptr);

    VkAttachmentDescription color{};
    color.format = swapFormat;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth{};
    depth.format = findDepthFormat();
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depthRef{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription sub{};
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments = &colorRef;
    sub.pDepthStencilAttachment = &depthRef;

    VkAttachmentDescription atts[2] = { color, depth };
    VkRenderPassCreateInfo rpci{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    rpci.attachmentCount = 2;
    rpci.pAttachments = atts;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &sub;
    if (vkCreateRenderPass(device, &rpci, nullptr, &renderPass) != VK_SUCCESS)
        throw std::runtime_error("renderPass");

    destroyDepthResources();
    createDepthResources();

    framebuffers.resize(ic);
    for (uint32_t i = 0; i < ic; ++i) {
        VkImageView atv[2] = { swapViews[i], depthView };
        VkFramebufferCreateInfo fci{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        fci.renderPass = renderPass;
        fci.attachmentCount = 2;
        fci.pAttachments = atv;
        fci.width = swapExtent.width;
        fci.height = swapExtent.height;
        fci.layers = 1;
        if (vkCreateFramebuffer(device, &fci, nullptr, &framebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("framebuffer");
    }
}

void VulkanContext::createSyncObjectsForSwapchain()
{
    for (auto s : imageAvailable)
        if (s)
            vkDestroySemaphore(device, s, nullptr);
    for (auto s : renderFinishedImg)
        if (s)
            vkDestroySemaphore(device, s, nullptr);
    for (auto f : inFlightFences)
        if (f)
            vkDestroyFence(device, f, nullptr);
    imageAvailable.clear();
    renderFinishedImg.clear();
    inFlightFences.clear();
    imagesInFlight.clear();

    const uint32_t imgCount = (uint32_t)swapImages.size();
    imagesInFlight.resize(imgCount, VK_NULL_HANDLE);
    renderFinishedImg.resize(imgCount, VK_NULL_HANDLE);

    imageAvailable.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

    VkSemaphoreCreateInfo sci{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VkFenceCreateInfo fci{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateSemaphore(device, &sci, nullptr, &imageAvailable[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fci, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("sync objs (per-frame)");
    }
    for (uint32_t i = 0; i < imgCount; ++i) {
        if (vkCreateSemaphore(device, &sci, nullptr, &renderFinishedImg[i]) != VK_SUCCESS)
            throw std::runtime_error("sync objs (per-image)");
    }
    currentFrame = 0;
}

void VulkanContext::cleanupSwapchain()
{
    if (!cmdBufs.empty()) {
        vkFreeCommandBuffers(device, cmdPool, (uint32_t)cmdBufs.size(), cmdBufs.data());
        cmdBufs.clear();
    }
    for (auto fb : framebuffers)
        vkDestroyFramebuffer(device, fb, nullptr);
    framebuffers.clear();
    destroyDepthResources();
    if (renderPass) {
        vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }
    for (auto v : swapViews)
        vkDestroyImageView(device, v, nullptr);
    swapViews.clear();
    if (swapchain) {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }
}

void VulkanContext::recreateSwapchain()
{
    vkDeviceWaitIdle(device);
    cleanupSwapchain();
    createSwapchain();
    allocCommandBuffersForSwapchain();
    createSyncObjectsForSwapchain();
    rebuildGraphicsPipeline();
    framebufferResized = false;
}

void VulkanContext::uploadMesh(const Mesh& m)
{
    for (auto& g : gpuChunks) {
        if (g.ibo) {
            vkDestroyBuffer(device, g.ibo, nullptr);
            g.ibo = {};
        }
        if (g.vbo) {
            vkDestroyBuffer(device, g.vbo, nullptr);
            g.vbo = {};
        }
        if (g.iboMem) {
            vkFreeMemory(device, g.iboMem, nullptr);
            g.iboMem = {};
        }
        if (g.vboMem) {
            vkFreeMemory(device, g.vboMem, nullptr);
            g.vboMem = {};
        }
    }
    gpuChunks.clear();
    chunkIndex.clear();

    VkDeviceSize vsize = sizeof(Vertex) * m.vertices.size();
    VkDeviceSize isize = sizeof(uint32_t) * m.indices.size();

    if (vbo) {
        vkDestroyBuffer(device, vbo, nullptr);
        vbo = {};
    }
    if (ibo) {
        vkDestroyBuffer(device, ibo, nullptr);
        ibo = {};
    }
    if (vboMem) {
        vkFreeMemory(device, vboMem, nullptr);
        vboMem = {};
    }
    if (iboMem) {
        vkFreeMemory(device, iboMem, nullptr);
        iboMem = {};
    }

    createBuffer(vsize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vbo,
                 vboMem);
    createBuffer(isize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ibo,
                 iboMem);

    VkBuffer sV{};
    VkDeviceMemory sVM{};
    createBuffer(vsize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sV,
                 sVM);
    void* p = nullptr;
    vkMapMemory(device, sVM, 0, vsize, 0, &p);
    std::memcpy(p, m.vertices.data(), (size_t)vsize);
    vkUnmapMemory(device, sVM);
    copyBuffer(sV, vbo, vsize);
    vkDestroyBuffer(device, sV, nullptr);
    vkFreeMemory(device, sVM, nullptr);

    VkBuffer sI{};
    VkDeviceMemory sIM{};
    createBuffer(isize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sI,
                 sIM);
    vkMapMemory(device, sIM, 0, isize, 0, &p);
    std::memcpy(p, m.indices.data(), (size_t)isize);
    vkUnmapMemory(device, sIM);
    copyBuffer(sI, ibo, isize);
    vkDestroyBuffer(device, sI, nullptr);
    vkFreeMemory(device, sIM, nullptr);

    indexCount = (uint32_t)m.indices.size();
}

void VulkanContext::uploadOne(const Mesh& m, GpuMesh& out)
{
    VkDeviceSize vsize = sizeof(Vertex) * m.vertices.size();
    VkDeviceSize isize = sizeof(uint32_t) * m.indices.size();

    createBuffer(vsize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, out.vbo,
                 out.vboMem);
    createBuffer(isize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, out.ibo,
                 out.iboMem);

    VkBuffer sV{};
    VkDeviceMemory sVM{};
    createBuffer(vsize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sV,
                 sVM);
    void* p = nullptr;
    vkMapMemory(device, sVM, 0, vsize, 0, &p);
    std::memcpy(p, m.vertices.data(), (size_t)vsize);
    vkUnmapMemory(device, sVM);
    copyBuffer(sV, out.vbo, vsize);
    vkDestroyBuffer(device, sV, nullptr);
    vkFreeMemory(device, sVM, nullptr);

    VkBuffer sI{};
    VkDeviceMemory sIM{};
    createBuffer(isize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sI,
                 sIM);
    vkMapMemory(device, sIM, 0, isize, 0, &p);
    std::memcpy(p, m.indices.data(), (size_t)isize);
    vkUnmapMemory(device, sIM);
    copyBuffer(sI, out.ibo, isize);
    vkDestroyBuffer(device, sI, nullptr);
    vkFreeMemory(device, sIM, nullptr);

    out.indexCount = (uint32_t)m.indices.size();

    glm::vec3 mn(1e9f), mx(-1e9f);
    for (auto& v : m.vertices) {
        mn = glm::min(mn, v.pos);
        mx = glm::max(mx, v.pos);
    }
    out.aabbMin = mn;
    out.aabbMax = mx;
}

void VulkanContext::uploadChunkMeshes(const std::vector<Mesh>& chunks)
{
    if (ibo) {
        vkDestroyBuffer(device, ibo, nullptr);
        ibo = {};
    }
    if (vbo) {
        vkDestroyBuffer(device, vbo, nullptr);
        vbo = {};
    }
    if (iboMem) {
        vkFreeMemory(device, iboMem, nullptr);
        iboMem = {};
    }
    if (vboMem) {
        vkFreeMemory(device, vboMem, nullptr);
        vboMem = {};
    }
    indexCount = 0;

    for (auto& g : gpuChunks) {
        if (g.ibo) {
            vkDestroyBuffer(device, g.ibo, nullptr);
            g.ibo = {};
        }
        if (g.vbo) {
            vkDestroyBuffer(device, g.vbo, nullptr);
            g.vbo = {};
        }
        if (g.iboMem) {
            vkFreeMemory(device, g.iboMem, nullptr);
            g.iboMem = {};
        }
        if (g.vboMem) {
            vkFreeMemory(device, g.vboMem, nullptr);
            g.vboMem = {};
        }
    }
    gpuChunks.clear();
    chunkIndex.clear();

    gpuChunks.resize(chunks.size());
    for (size_t i = 0; i < chunks.size(); ++i)
        uploadOne(chunks[i], gpuChunks[i]);
}

static int64_t packCC(const ChunkCoord& c)
{
    return packKey(c);
}

static void createOrReplace(VulkanContext* self, int64_t key, const Mesh& m, size_t& outIndex)
{
    auto it = self->chunkIndex.find(key);
    if (it == self->chunkIndex.end()) {
        self->gpuChunks.emplace_back();
        size_t idx = self->gpuChunks.size() - 1;
        self->uploadOne(m, self->gpuChunks[idx]);
        self->gpuChunks[idx].key = key;
        self->chunkIndex[key] = idx;
        outIndex = idx;
    } else {
        size_t idx = it->second;
        auto& g = self->gpuChunks[idx];
        if (g.ibo) {
            vkDestroyBuffer(self->device, g.ibo, nullptr);
            g.ibo = {};
        }
        if (g.vbo) {
            vkDestroyBuffer(self->device, g.vbo, nullptr);
            g.vbo = {};
        }
        if (g.iboMem) {
            vkFreeMemory(self->device, g.iboMem, nullptr);
            g.iboMem = {};
        }
        if (g.vboMem) {
            vkFreeMemory(self->device, g.vboMem, nullptr);
            g.vboMem = {};
        }
        self->uploadOne(m, g);
        outIndex = idx;
    }
}

void VulkanContext::syncChunks(const std::vector<ChunkCoord>& keepVisible, const std::vector<std::pair<ChunkCoord, Mesh>>& dirty)
{
    for (auto& pair : dirty) {
        const auto& cc = pair.first;
        const Mesh& m = pair.second;
        size_t idx = 0;
        createOrReplace(this, packCC(cc), m, idx);
    }

    std::unordered_set<int64_t, PackedKeyHash> keep;
    keep.reserve(keepVisible.size());
    for (auto& cc : keepVisible)
        keep.insert(packCC(cc));

    for (int i = int(gpuChunks.size()) - 1; i >= 0; --i) {
        if (keep.find(gpuChunks[i].key) != keep.end())
            continue;

        auto& g = gpuChunks[i];
        if (g.ibo)
            vkDestroyBuffer(device, g.ibo, nullptr);
        if (g.vbo)
            vkDestroyBuffer(device, g.vbo, nullptr);
        if (g.iboMem)
            vkFreeMemory(device, g.iboMem, nullptr);
        if (g.vboMem)
            vkFreeMemory(device, g.vboMem, nullptr);

        chunkIndex.erase(g.key);
        if (i != int(gpuChunks.size()) - 1) {
            gpuChunks[i] = gpuChunks.back();
            chunkIndex[gpuChunks[i].key] = size_t(i);
        }
        gpuChunks.pop_back();
    }
}

void VulkanContext::loadAtlasPNG(const char* relPath)
{
    auto full = assetPath(relPath);
    int w, h, ch;
    stbi_uc* pixels = stbi_load(full.c_str(), &w, &h, &ch, STBI_rgb_alpha);
    if (!pixels)
        throw std::runtime_error(std::string("Failed to load atlas: ") + full);
    VkDeviceSize size = (VkDeviceSize)w * h * 4;

    VkBuffer staging;
    VkDeviceMemory stagingMem;
    createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 staging, stagingMem);

    void* p = nullptr;
    vkMapMemory(device, stagingMem, 0, size, 0, &p);
    std::memcpy(p, pixels, (size_t)size);
    vkUnmapMemory(device, stagingMem);
    stbi_image_free(pixels);

    if (atlasView) {
        vkDestroyImageView(device, atlasView, nullptr);
        atlasView = {};
    }
    if (atlasImage) {
        vkDestroyImage(device, atlasImage, nullptr);
        atlasImage = {};
    }
    if (atlasMem) {
        vkFreeMemory(device, atlasMem, nullptr);
        atlasMem = {};
    }

    createImage((uint32_t)w, (uint32_t)h, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, atlasImage, atlasMem);

    transitionImageLayout(atlasImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(staging, atlasImage, (uint32_t)w, (uint32_t)h);
    transitionImageLayout(atlasImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, stagingMem, nullptr);

    atlasView = createImageView(atlasImage, VK_FORMAT_R8G8B8A8_SRGB);

    VkDescriptorImageInfo di{ sampler, atlasView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    VkWriteDescriptorSet wds{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    wds.dstSet = descSet;
    wds.dstBinding = 0;
    wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    wds.descriptorCount = 1;
    wds.pImageInfo = &di;
    vkUpdateDescriptorSets(device, 1, &wds, 0, nullptr);
}

void VulkanContext::drawFrame(const void* pcData, size_t pcSize)
{
    VkFence frameFence = inFlightFences[currentFrame];
    vkWaitForFences(device, 1, &frameFence, VK_TRUE, UINT64_MAX);

    uint32_t imgIndex;
    VkResult acq = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailable[currentFrame], VK_NULL_HANDLE, &imgIndex);
    if (acq == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }
    if (acq != VK_SUCCESS && acq != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("acquire");

    if (imagesInFlight[imgIndex] != VK_NULL_HANDLE)
        vkWaitForFences(device, 1, &imagesInFlight[imgIndex], VK_TRUE, UINT64_MAX);
    imagesInFlight[imgIndex] = frameFence;
    vkResetFences(device, 1, &frameFence);

    VkCommandBuffer cb = cmdBufs[imgIndex];
    vkResetCommandBuffer(cb, 0);
    VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    vkBeginCommandBuffer(cb, &bi);

    VkClearValue clears[2];
    clears[0].color = { { 0.24f, 0.46f, 0.86f, 1.f } };
    clears[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo rpb{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    rpb.renderPass = renderPass;
    rpb.framebuffer = framebuffers[imgIndex];
    rpb.renderArea.extent = swapExtent;
    rpb.clearValueCount = 2;
    rpb.pClearValues = clears;
    vkCmdBeginRenderPass(cb, &rpb, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport vp{ 0, 0, (float)swapExtent.width, (float)swapExtent.height, 0.0f, 1.0f };
    VkRect2D sc{ { 0, 0 }, swapExtent };
    vkCmdSetViewport(cb, 0, 1, &vp);
    vkCmdSetScissor(cb, 0, 1, &sc);

    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSet, 0, nullptr);
    vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, (uint32_t)pcSize, pcData);

    if (!gpuChunks.empty()) {
        glm::mat4 VP = *reinterpret_cast<const glm::mat4*>(pcData);
        Frustum fr = extractFrustum(VP);

        for (auto& g : gpuChunks) {
            if (g.indexCount == 0)
                continue;
            if (!aabbInFrustum(fr, g.aabbMin, g.aabbMax))
                continue;
            VkDeviceSize offs = 0;
            vkCmdBindVertexBuffers(cb, 0, 1, &g.vbo, &offs);
            vkCmdBindIndexBuffer(cb, g.ibo, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cb, g.indexCount, 1, 0, 0, 0);
        }
    } else if (indexCount > 0) {
        VkDeviceSize offs = 0;
        vkCmdBindVertexBuffers(cb, 0, 1, &vbo, &offs);
        vkCmdBindIndexBuffer(cb, ibo, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cb, indexCount, 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(cb);
    vkEndCommandBuffer(cb);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &imageAvailable[currentFrame];
    si.pWaitDstStageMask = &waitStage;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &cb;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &renderFinishedImg[imgIndex];

    if (vkQueueSubmit(queue, 1, &si, frameFence) != VK_SUCCESS)
        throw std::runtime_error("submit");

    VkPresentInfoKHR pi{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &renderFinishedImg[imgIndex];
    pi.swapchainCount = 1;
    pi.pSwapchains = &swapchain;
    pi.pImageIndices = &imgIndex;

    VkResult pres = vkQueuePresentKHR(queue, &pi);
    if (pres == VK_ERROR_OUT_OF_DATE_KHR || pres == VK_SUBOPTIMAL_KHR || framebufferResized) {
        recreateSwapchain();
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanContext::cleanup()
{
    vkDeviceWaitIdle(device);

    for (auto& g : gpuChunks) {
        if (g.ibo)
            vkDestroyBuffer(device, g.ibo, nullptr);
        if (g.vbo)
            vkDestroyBuffer(device, g.vbo, nullptr);
        if (g.iboMem)
            vkFreeMemory(device, g.iboMem, nullptr);
        if (g.vboMem)
            vkFreeMemory(device, g.vboMem, nullptr);
    }
    gpuChunks.clear();
    chunkIndex.clear();

    if (ibo)
        vkDestroyBuffer(device, ibo, nullptr);
    if (vbo)
        vkDestroyBuffer(device, vbo, nullptr);
    if (iboMem)
        vkFreeMemory(device, iboMem, nullptr);
    if (vboMem)
        vkFreeMemory(device, vboMem, nullptr);

    if (atlasView)
        vkDestroyImageView(device, atlasView, nullptr);
    if (atlasImage)
        vkDestroyImage(device, atlasImage, nullptr);
    if (atlasMem)
        vkFreeMemory(device, atlasMem, nullptr);
    if (sampler)
        vkDestroySampler(device, sampler, nullptr);
    if (descPool)
        vkDestroyDescriptorPool(device, descPool, nullptr);
    if (setLayout)
        vkDestroyDescriptorSetLayout(device, setLayout, nullptr);

    cleanupSwapchain();

    for (auto s : imageAvailable)
        if (s)
            vkDestroySemaphore(device, s, nullptr);
    for (auto s : renderFinishedImg)
        if (s)
            vkDestroySemaphore(device, s, nullptr);
    for (auto f : inFlightFences)
        if (f)
            vkDestroyFence(device, f, nullptr);

    vkDestroyCommandPool(device, cmdPool, nullptr);
    if (pipeline) {
        vkDestroyPipeline(device, pipeline, nullptr);
    }
    if (pipelineLayout) {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}
