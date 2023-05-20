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

struct calcData {
	uint32_t circleSolutionValue;
	uint32_t eigthDiskSolutionValue;
};

struct cellData {
	uint32_t command;
	uint32_t species;
	uint32_t state;
	uint32_t atractivity;
	uint32_t delay;
};


struct InputData {
	int Xres;          // 0 4 bytes
	int Yres;          // 1 4 bytes
	int Wish;          // 2 4 bytes
	int Stay;          // 3 4 bytes
	int maxCombiSum;	// 4 4 bytes
	int anzahlCluster;	// 5 4 bytes
	int thisSpecies;	// 6 4 bytes
	int stateGroups;	// 7 4 bytes
	int testint;		// 8 4 bytes	
	float factorPart;	// 9 4 bytes	
	int pad_0;
	int pad_1;
	int BodyDNA[96];	// 10 4 bytes	
	int RulesDNA[4];   // 11 16 bytes
	int padding_0[2]; // 8 bytes padding
	int sensorGruppenAnzahl[6]; // 24 bytes
	int SensorGruppenIDs[96]; // 384 bytes
	int Wertigkeiten[6];
	int speciesRetardValues[18];
	int speciesThresh[18];
	int speciesAcc[18];
	bool square;
	bool sumTest;
};



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

struct frameSetGroup {
	VkDescriptorSet storageSet;
	VkDescriptorSet uniformSet;
	VkDescriptorSet* arr;
};

struct VulkanPipeline {
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet* descriptorSets;
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo;
	
	VkDescriptorPool uniformsDescriptorPool;
	VkDescriptorSetLayout uniformsDescriptorSetLayout;
	VkDescriptorSet* uniformsDescriptorSets;
	VkDescriptorSetLayoutCreateInfo uniformsLayoutCreateInfo;
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
void uploadDataToBuffer(VulkanContext* context, VulkanBuffer* dst_buffer, void* data, size_t size, VulkanBuffer* src_buffer = nullptr);
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
	VkDescriptorBufferInfo* bufferInfos_1,
	VkDescriptorBufferInfo* bufferInfos_2,
	uint32_t numDescriptorSets,
	uint32_t descriptorSetSize_1,
	uint32_t descriptorSetSize_2,
	uint32_t pushDataSize,
	VkPipelineCreateFlags flags,
	VkSpecializationInfo* specializationInfo,
	uint32_t amountExtraSingleStoragesPerSet,
	VkDescriptorBufferInfo* extraSingleBufferInfos,
	uint32_t amountExtraSingleUniformsPerSet,
	VkDescriptorBufferInfo* extraSingleUniformBufferInfos);

void destroyComputePipeline(VulkanContext* context, VulkanPipeline* pipeline);


