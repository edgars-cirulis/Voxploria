#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Mesh.hpp"

struct GpuMesh {
    VkBuffer vbo{ VK_NULL_HANDLE };
    VkBuffer ibo{ VK_NULL_HANDLE };
    VkDeviceMemory vboMem{ VK_NULL_HANDLE };
    VkDeviceMemory iboMem{ VK_NULL_HANDLE };
    uint32_t indexCount{ 0 };
    glm::vec3 aabbMin{ 0.0f };
    glm::vec3 aabbMax{ 0.0f };
};

struct VulkanContext {
    GLFWwindow* window{};
    VkInstance instance{};
    VkSurfaceKHR surface{};
    VkPhysicalDevice phys{};
    VkDevice device{};
    uint32_t queueFamily{};
    VkQueue queue{};

    VkSwapchainKHR swapchain{};
    VkFormat swapFormat{};
    VkExtent2D swapExtent{};
    std::vector<VkImage> swapImages;
    std::vector<VkImageView> swapViews;
    VkRenderPass renderPass{};
    std::vector<VkFramebuffer> framebuffers;

    VkImage depthImage{};
    VkDeviceMemory depthMem{};
    VkImageView depthView{};

    VkCommandPool cmdPool{};
    std::vector<VkCommandBuffer> cmdBufs;

    const int MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> imageAvailable;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    std::vector<VkSemaphore> renderFinishedImg;
    size_t currentFrame = 0;

    VkPipelineLayout pipelineLayout{};
    VkPipeline pipeline{};

    VkDescriptorSetLayout setLayout{};
    VkDescriptorPool descPool{};
    VkDescriptorSet descSet{};
    VkSampler sampler{};
    VkImage atlasImage{};
    VkDeviceMemory atlasMem{};
    VkImageView atlasView{};

    VkBuffer vbo{ VK_NULL_HANDLE }, ibo{ VK_NULL_HANDLE };
    VkDeviceMemory vboMem{ VK_NULL_HANDLE }, iboMem{ VK_NULL_HANDLE };
    uint32_t indexCount{ 0 };

    std::vector<GpuMesh> gpuChunks;

    void init(GLFWwindow* win, int width, int height);
    void cleanup();

    void drawFrame(const void* pcData, size_t pcSize);

    void uploadMesh(const Mesh& m);
    void uploadChunkMeshes(const std::vector<Mesh>&);
    void loadAtlasPNG(const char* path);

    void requestResize() { framebufferResized = true; }
    VkExtent2D getExtent() const { return swapExtent; }

   private:
    VkPhysicalDeviceMemoryProperties memProps{};
    uint32_t findMemType(uint32_t typeBits, VkMemoryPropertyFlags flags);

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkBuffer& buf, VkDeviceMemory& mem);
    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

    void createImage(uint32_t w,
                     uint32_t h,
                     VkFormat fmt,
                     VkImageTiling tiling,
                     VkImageUsageFlags usage,
                     VkMemoryPropertyFlags props,
                     VkImage& img,
                     VkDeviceMemory& mem);
    VkImageView createImageView(VkImage img, VkFormat fmt);
    void transitionImageLayout(VkImage img, VkFormat fmt, VkImageLayout oldL, VkImageLayout newL);
    void copyBufferToImage(VkBuffer buf, VkImage img, uint32_t w, uint32_t h);

    VkFormat findDepthFormat();
    void createDepthResources();
    void destroyDepthResources();

    bool framebufferResized = false;
    void cleanupSwapchain();
    void createSwapchain();
    void recreateSwapchain();
    void allocCommandBuffersForSwapchain();
    void createSyncObjectsForSwapchain();

    void rebuildGraphicsPipeline();

    void uploadOne(const Mesh& m, GpuMesh& out);
};
