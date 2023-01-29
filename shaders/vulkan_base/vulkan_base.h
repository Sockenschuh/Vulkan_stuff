#include "../logger.h"

#include <vulkan/vulkan.h>
#include <cassert>
#include <vector>

#define ASSERT_VULKAN(val) if(val != VK_SUCCESS) {assert(false);}
#ifndef VK
#define VK(f) (f)
#endif
#ifndef VKA
#define VKA(f) ASSERT_VULKAN(VK(f))
#endif

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

struct VulkanQueue {
	VkQueue queue;
	uint32_t familyIndex;
};

struct VulkanSwapchain {
	VkSwapchainKHR swapchain;
	uint32_t width;
	uint32_t height;
	VkFormat format;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
};

struct VulkanPipeline {
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet* descriptorSets;
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo;
};

struct VulkanContext {
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceProperties physicalDeviceProperties;
	VkDevice device;
	VulkanQueue graphicsQueue;
	VulkanQueue computeQueue;
	VkDebugUtilsMessengerEXT debugCallback;
};

struct VulkanBuffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
	uint32_t offset;
};

VulkanContext* initVulkan(uint32_t instanceExtensionCount, const char** instanceExtensions, uint32_t deviceExtensionCount, const char** deviceExtensions);
void exitVulkan(VulkanContext* context);

VulkanSwapchain createSwapchain(VulkanContext* context, VkSurfaceKHR surface, VkImageUsageFlags usage, VulkanSwapchain* oldSwapchain = 0);
void destroySwapchain(VulkanContext* context, VulkanSwapchain* swapchain);

VkRenderPass createRenderPass(VulkanContext* context, VkFormat format);
void destroyRenderpass(VulkanContext* context, VkRenderPass renderPass);

void createBuffer(VulkanContext* context, VulkanBuffer* buffer, uint64_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
void uploadDataToBuffer(VulkanContext* context, VulkanBuffer* buffer, void* data, size_t size);
void destroyBuffer(VulkanContext* context, VulkanBuffer* buffer);

VulkanPipeline createGraphicsPipeline(VulkanContext* context,
	const char* vertexShaderFilename,
	const char* fragmentShaderFilename,
	VkRenderPass renderPass,
	uint32_t width,
	uint32_t height,
	VkVertexInputAttributeDescription* attributes,
	uint32_t numAttributes,
	VkVertexInputBindingDescription* binding,
	uint32_t numReadComputeStorages,
	uint32_t numDescriptorSets,
	uint32_t pushDataSize,
	VkDescriptorBufferInfo* bufferInfos);

void destroyGraphicsPipeline(VulkanContext* context, VulkanPipeline* pipeline);

VulkanPipeline createComputePipeline(VulkanContext* context,
	const char* computeShaderFilename,
	VkDescriptorBufferInfo* bufferInfos,
	uint32_t numDescriptorSets,
	uint32_t descriptorSetSize,
	uint32_t pushDataSize,
	VkPipelineCreateFlags flags = 0,
	VkSpecializationInfo* specializationInfo = nullptr);

void destroyComputePipeline(VulkanContext* context, VulkanPipeline* pipeline);
