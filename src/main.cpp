#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "logger.h"
#include "vulkan_base/vulkan_base.h"

#include <random>
#include <chrono>
#include <iostream>

std::mt19937 randomEngine(std::random_device{}());
std::uniform_int_distribution<uint32_t> distribution(0, 1);


#define FRAMES_IN_FLIGHT 4
#define computeDescriptorSetSize 2
//#define VULKAN_TIME_PROFILE

VulkanContext* context;
VkSurfaceKHR surface;
VulkanSwapchain swapchain;
VkRenderPass renderPass;
std::vector<VkFramebuffer> framebuffers;
VulkanPipeline graphicsPipeline;
VkCommandPool commandPools[FRAMES_IN_FLIGHT];
VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];
VkFence fencesGraphics[FRAMES_IN_FLIGHT];
VkFence fencesCompute[FRAMES_IN_FLIGHT];
VkSemaphore acquireSemaphores[FRAMES_IN_FLIGHT];
//VkSemaphore releaseSemaphores[FRAMES_IN_FLIGHT];
VkSemaphore computeFinishedSemaphoreForCompute[FRAMES_IN_FLIGHT];
VkSemaphore computeFinishedSemaphoreForGraphics[FRAMES_IN_FLIGHT];
VkSemaphore renderFinishedSemaphoreForPresent[FRAMES_IN_FLIGHT];
VkSemaphore renderFinishedSemaphoreForCompute[FRAMES_IN_FLIGHT];
VulkanBuffer vertexBuffer;
VulkanBuffer indexBuffer;




VulkanBuffer cellStateBuffers[FRAMES_IN_FLIGHT];

VulkanBuffer initialCellStateBuffer;
VkCommandBuffer initialCellStateCommandBuffer;
VkCommandPool initialCellStateCommandBufferPool;

VkDescriptorBufferInfo bufferInfos[FRAMES_IN_FLIGHT];
VulkanPipeline computePipeline;

VkCommandBuffer computeCommandBuffers[FRAMES_IN_FLIGHT];
VkCommandPool computeCommandPools[FRAMES_IN_FLIGHT];


const uint32_t Xres = 32 * 100;
const uint32_t Yres = 32 * 100;
uint32_t bufferSize = Xres * Yres * sizeof(uint32_t);
uint32_t initialData[Xres * Yres];

uint32_t offsets[2][2] = {
	{0, Xres * Yres * sizeof(uint32_t)},
	{Xres * Yres * sizeof(uint32_t), 0}
};

struct PushData {
	uint32_t width;
	uint32_t height;
};


bool handleMessage() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			return false;
		}
	}
	return true;
}

void recreateRenderPass() {
	if(renderPass) {
		for (uint32_t i = 0; i < framebuffers.size(); ++i) {
			VK(vkDestroyFramebuffer(context->device, framebuffers[i], 0));
		}
		destroyRenderpass(context, renderPass);
	}
	framebuffers.clear();

	renderPass = createRenderPass(context, swapchain.format);
	framebuffers.resize(swapchain.images.size());
	for (uint32_t i = 0; i < swapchain.images.size(); ++i) {
		VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		createInfo.renderPass = renderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &swapchain.imageViews[i];
		createInfo.width = swapchain.width;
		createInfo.height = swapchain.height;
		createInfo.layers = 1;
		VKA(vkCreateFramebuffer(context->device, &createInfo, 0, &framebuffers[i]));
	}
}

float vertexData[] = {
	0.995f*2*0.5f,0.995f * 2*-0.5f,
	1.0f, 0.0f, 0.0f,

	0.995f * 2*0.5f, 0.995f * 2*0.5f,
	0.0f, 1.0f, 0.0f,

	0.995f * 2*-0.5f, 0.995f * 2*0.5f,
	0.0f, 0.0f, 1.0f,

	0.995f * 2*-0.5f, 0.995f * 2*-0.5f,
	0.0f, 1.0f, 0.0f,
};

uint32_t indexData[] = {
	0, 1, 2,
	3, 0, 2,
};

void initCellstates() {
	for (size_t x = 0; x < Xres; x++)
	{
		for (size_t y = 0; y < Yres; y++)
		{
			/*if (x % 16 == 8) {
				initialData[y * Xres + x] = 13;
			}
			if (x % 16 == 10) {
				initialData[y * Xres + x] = 13;
			}
			if (y % 16 == 8) {
				initialData[y * Xres + x] = 13;
			}
			if (x % 8 == 4 && y % 8 == 4) {
				initialData[y * Xres + x] = 13;
			}
			if (x % 8 == 5 && y % 8 == 4) {
				initialData[y * Xres + x] = 13;
			}
			if (x % 8 == 5 && y % 8 == 5 && x%3 ==2) {
				initialData[y * Xres + x] = 13;
			}
			if (x % 8 == 6 && y % 6 == 5 && x % 3 == 2) {
				initialData[y * Xres + x] = 13;
			}*/
			if (x % 8 == 5 && y % 8 == 6 && distribution(randomEngine) * distribution(randomEngine) == 1) {

				initialData[y * Xres + x] = distribution(randomEngine) * 10;
			}

			if (x == 0 || y == 0) {
				initialData[y * Xres + x] = 13;
			}
			//initialData[y * Xres + x] = 0;
			//initialData[y * Xres + x] = distribution(randomEngine)*10;

		}
	}

	/*for (int i = 0; i < Xres * Yres; i++) {

		initialData[i] = 0;


		if ( i >= 2 * Xres) {
			if (i % 2 == 0) {
				initialData[i] = 10;
			}
			if (i % 2 == 0) {

				initialData[i] = 10;
			}
		}
		else {
			if (i % Xres < (Xres / 2) - 3) {
				if (i % 2 == 1) {
					initialData[i] = 10;
				}
			}
		}
		if (i == (Xres * Yres / 2) + (Xres / 2)) {
			initialData[i] = 0;
		}
		if (i+1 == (Xres * Yres / 2) + (Xres / 2)) {
			initialData[i] = 0;
		}
		if (i+2 == (Xres * Yres / 2) + (Xres / 2)) {
			initialData[i] = 0;
		}
		if (i+3 == (Xres * Yres / 2) + (Xres / 2)) {
			initialData[i] = 0;
		}
		if (i - 1 == (Xres * Yres / 2) + (Xres / 2)) {
			initialData[i] = 0;
		}
		if (i - 2 == (Xres * Yres / 2) + (Xres / 2)) {
			initialData[i] = 0;
		}
		if (i - 3 == (Xres * Yres / 2) + (Xres / 2)) {
			initialData[i] = 0;
		}
		if (i+Xres == (Xres * Yres / 2) + (Xres / 2)) {
			initialData[i] = 0;
		}
		if (i+1+Xres == (Xres * Yres / 2) + (Xres / 2)) {
			initialData[i] = 0;
		}


		//initialData[i] = distribution(randomEngine);

	}*/

	//create Hostvisible Buffer and fill with initial Data
	/*createBuffer(context, &initialCellStateBuffer, Xres * Yres * sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void* mappedMemory;
	vkMapMemory(context->device, initialCellStateBuffer.memory, 0, Xres * Yres * sizeof(uint32_t), 0, &mappedMemory);
	memcpy(mappedMemory, initialData, Xres * Yres * sizeof(uint32_t));
	vkUnmapMemory(context->device, initialCellStateBuffer.memory);

	//create Pool and Command Buffer for initial single use
	VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	createInfo.queueFamilyIndex = context->computeQueue.familyIndex;
	VKA(vkCreateCommandPool(context->device, &createInfo, 0, &initialCellStateCommandBufferPool));
	VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandPool = initialCellStateCommandBufferPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;
	VKA(vkAllocateCommandBuffers(context->device, &allocateInfo, &initialCellStateCommandBuffer));*/

	// Copy the data from the host-visible buffer to the GPU-internal stateBuffers
	{
		//set the region to copy
		VkBufferCopy region = { 0, 0, Xres * Yres * sizeof(uint32_t) };


		for (size_t i = 0; i < ARRAY_COUNT(cellStateBuffers); i++)
		{

			uploadDataToBuffer(context, &cellStateBuffers[i], initialData, Xres* Yres * sizeof(uint32_t));

			/*//wait for idle
			VKA(vkDeviceWaitIdle(context->device));

			VKA(vkResetCommandPool(context->device, initialCellStateCommandBufferPool, 0));
			VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			//start recording
			vkBeginCommandBuffer(initialCellStateCommandBuffer, &beginInfo);

			vkCmdCopyBuffer(initialCellStateCommandBuffer, initialCellStateBuffer.buffer, cellStateBuffers[i].buffer, 1, &region);

			//end recording
			vkEndCommandBuffer(initialCellStateCommandBuffer);


			//submit
			VkPipelineStageFlags pComputeWaitMasks[2] = { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
			VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &initialCellStateCommandBuffer;

			vkQueueSubmit(context->computeQueue.queue, 1, &submitInfo, 0);*/
		}
	}
	VKA(vkDeviceWaitIdle(context->device));
}

void initCompute() {

	
	PushData pushData;

	computePipeline = createComputePipeline(context, "../shaders/gameOfLife_comp.spv", bufferInfos, FRAMES_IN_FLIGHT, computeDescriptorSetSize, sizeof(PushData), 0, 0);

	for (uint32_t i = 0; i < ARRAY_COUNT(computeCommandPools); ++i) {
		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		createInfo.queueFamilyIndex = context->computeQueue.familyIndex;
		VKA(vkCreateCommandPool(context->device, &createInfo, 0, &computeCommandPools[i]));
	}

	for (uint32_t i = 0; i < ARRAY_COUNT(computeCommandPools); ++i) {
		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = computeCommandPools[i];
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		VKA(vkAllocateCommandBuffers(context->device, &allocateInfo, &computeCommandBuffers[i]));
	}

}

void initApplication(SDL_Window* window) {
	const char* additionalInstanceExtensions[] = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
	};
	uint32_t instanceExtensionCount;
	SDL_Vulkan_GetInstanceExtensions(&instanceExtensionCount, 0);
	const char** enabledInstanceExtensions = new const char* [instanceExtensionCount + ARRAY_COUNT(additionalInstanceExtensions)];
	SDL_Vulkan_GetInstanceExtensions(&instanceExtensionCount, enabledInstanceExtensions);
	for (uint32_t i = 0; i < ARRAY_COUNT(additionalInstanceExtensions); ++i) {
		enabledInstanceExtensions[instanceExtensionCount++] = additionalInstanceExtensions[i];
	}

	const char* enabledDeviceExtensions[]{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	context = initVulkan(instanceExtensionCount, enabledInstanceExtensions, ARRAY_COUNT(enabledDeviceExtensions), enabledDeviceExtensions);

	SDL_Vulkan_CreateSurface(window, context->instance, &surface);
	swapchain = createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

	recreateRenderPass();

	//create internal buffers
	uint32_t offset = 0;
	for (uint32_t i = 0; i < ARRAY_COUNT(cellStateBuffers); i++) {
		cellStateBuffers[i].offset = offset;
		createBuffer(context, &cellStateBuffers[i], Xres * Yres * sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); //VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		bufferInfos[i].buffer = cellStateBuffers[i].buffer;
		bufferInfos[i].offset = 0;
		bufferInfos[i].range = bufferSize;
		offset += bufferSize;
	}
	
	VkVertexInputAttributeDescription vertexAttributeDescriptions[2] = {};
	vertexAttributeDescriptions[0].binding = 0;
	vertexAttributeDescriptions[0].location = 0;
	vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescriptions[0].offset = 0;
	vertexAttributeDescriptions[1].binding = 0;
	vertexAttributeDescriptions[1].location = 1;
	vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[1].offset = sizeof(float) * 2;
	VkVertexInputBindingDescription vertexInputBinding = {};
	vertexInputBinding.binding = 0;
	vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexInputBinding.stride = sizeof(float) * 5;
	graphicsPipeline = createGraphicsPipeline(context, "../shaders/readBuffer_vert.spv", "../shaders/readBuffer_frag.spv", renderPass, swapchain.width, swapchain.height, vertexAttributeDescriptions, ARRAY_COUNT(vertexAttributeDescriptions), &vertexInputBinding, computeDescriptorSetSize, FRAMES_IN_FLIGHT, sizeof(PushData), bufferInfos);

	for(uint32_t i = 0; i < ARRAY_COUNT(fencesCompute); ++i) {
		VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VKA(vkCreateFence(context->device, &createInfo, 0, &fencesGraphics[i]));
		VKA(vkCreateFence(context->device, &createInfo, 0, &fencesCompute[i]));
	}
	for(uint32_t i = 0; i < ARRAY_COUNT(acquireSemaphores); ++i) {
		VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VKA(vkCreateSemaphore(context->device, &createInfo, 0, &acquireSemaphores[i]));
		VKA(vkCreateSemaphore(context->device, &createInfo, 0, &renderFinishedSemaphoreForPresent[i]));
		VKA(vkCreateSemaphore(context->device, &createInfo, 0, &renderFinishedSemaphoreForCompute[i]));
		VKA(vkCreateSemaphore(context->device, &createInfo, 0, &computeFinishedSemaphoreForCompute[i]));
		VKA(vkCreateSemaphore(context->device, &createInfo, 0, &computeFinishedSemaphoreForGraphics[i]));
		//VKA(vkCreateSemaphore(context->device, &createInfo, 0, &releaseSemaphores[i]));
	}

	for(uint32_t i = 0; i < ARRAY_COUNT(commandPools); ++i) {
		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		createInfo.queueFamilyIndex = context->graphicsQueue.familyIndex;
		VKA(vkCreateCommandPool(context->device, &createInfo, 0, &commandPools[i]));
	}
	for(uint32_t i = 0; i < ARRAY_COUNT(commandPools); ++i) {
		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = commandPools[i];
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		VKA(vkAllocateCommandBuffers(context->device, &allocateInfo, &commandBuffers[i]));
	}

	createBuffer(context, &vertexBuffer, sizeof(vertexData), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	uploadDataToBuffer(context, &vertexBuffer, vertexData, sizeof(vertexData));
	
	createBuffer(context, &indexBuffer, sizeof(indexData), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	uploadDataToBuffer(context, &indexBuffer, indexData, sizeof(indexData));

	initCompute();

	initCellstates();
}

void recreateSwapchain() {
	VulkanSwapchain oldSwapchain = swapchain;

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VKA(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, surface, &surfaceCapabilities));
	if(surfaceCapabilities.currentExtent.width == 0 || surfaceCapabilities.currentExtent.height == 0) {
		return;
	}


	VKA(vkDeviceWaitIdle(context->device));
	swapchain = createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &oldSwapchain);

	destroySwapchain(context, &oldSwapchain);
	recreateRenderPass();
}

float a = 0.0f;


void renderApplication() {
	static float greenChannel = 0.0f;
	a += 0.01f;
	greenChannel = pow(sin(a),100);
	if (greenChannel > 1.0f) greenChannel = 0.0f;
	uint32_t imageIndex = 0;
	static uint32_t frameIndex = 0;
#ifdef VULKAN_TIME_PROFILE
	auto start = std::chrono::high_resolution_clock::now();
	auto total_start = std::chrono::high_resolution_clock::now();
#endif
	// Wait for the n-2 frame to finish to be able to reuse its acquireSemaphore in vkAcquireNextImageKHR
	// Acquire Image	
	VKA(vkWaitForFences(context->device, 1, &fencesGraphics[frameIndex], VK_TRUE, UINT64_MAX));

	VkResult result = VK(vkAcquireNextImageKHR(context->device, swapchain.swapchain, UINT64_MAX, acquireSemaphores[frameIndex], 0, &imageIndex));
	if(result == VK_ERROR_OUT_OF_DATE_KHR ){// || result == VK_SUBOPTIMAL_KHR) {
		// Swapchain is out of date
		recreateSwapchain();
		VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VK(vkDestroySemaphore(context->device, acquireSemaphores[frameIndex], 0));
		VKA(vkCreateSemaphore(context->device, &createInfo, 0, &acquireSemaphores[frameIndex]));
		return;
	} else {
		VKA(vkResetFences(context->device, 1, &fencesCompute[frameIndex]));
		VKA(vkResetFences(context->device, 1, &fencesGraphics[frameIndex]));
		if (result != VK_SUBOPTIMAL_KHR) {
			ASSERT_VULKAN(result);
		}
	}
#ifdef VULKAN_TIME_PROFILE
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	std::cout << "Time taken to acquire image: \t\t\t\t" << duration << " microseconds" << std::endl;

	start = std::chrono::high_resolution_clock::now();
#endif
	//reset Pools Graphics
	VKA(vkResetCommandPool(context->device, commandPools[frameIndex], 0));
	//reset Pools compute
	VKA(vkResetCommandPool(context->device, computeCommandPools[frameIndex], 0));


	// Initialize VkCommandBufferBeginInfo struct for graphicsPipeline
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	// Set flags for command buffer usage
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	{
		// Get command buffer from commandBuffers array
		VkCommandBuffer commandBuffer = commandBuffers[frameIndex];

		// Begin command buffer
		VKA(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		// Set viewport and scissor
		VkViewport viewport = { 0.0f, 0.0f, (float)swapchain.width, (float)swapchain.height };
		VkRect2D scissor = { {0, 0}, {swapchain.width, swapchain.height} };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		// Set clear value
		VkClearValue clearValue = { 0.0f, greenChannel, 1.0f, 1.0f };

		// Initialize VkRenderPassBeginInfo struct
		VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		// Set render pass and framebuffer
		beginInfo.renderPass = renderPass;
		beginInfo.framebuffer = framebuffers[imageIndex];
		// Set render area
		beginInfo.renderArea = { {0, 0}, {swapchain.width, swapchain.height} };
		// Set clear value count and pointer
		beginInfo.clearValueCount = 1;
		beginInfo.pClearValues = &clearValue;

		// Begin render pass
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Bind graphics pipeline
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipeline);

		// Create vector of write descriptor sets
		std::vector<VkWriteDescriptorSet> writeDescriptorSets(computeDescriptorSetSize);

		// Bind descriptor sets
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout, 0, 1, &graphicsPipeline.descriptorSets[(frameIndex)], 0, 0);

		// Set offset for vertex buffer
		VkDeviceSize offset = 0;

		// Bind vertex and index buffers
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		// Create push data
		PushData push = { Xres, Yres };

		// Push constants
		vkCmdPushConstants(commandBuffer, graphicsPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushData), &push);

		// Draw indexed
		vkCmdDrawIndexed(commandBuffer, ARRAY_COUNT(indexData), 1, 0, 0, 0);

		// End render pass
		vkCmdEndRenderPass(commandBuffer);

		// End command buffer
		VKA(vkEndCommandBuffer(commandBuffer));
	}
#ifdef VULKAN_TIME_PROFILE
	end = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	std::cout << "Time taken to record graphics pipeline command buffer: \t" << duration << " microseconds" << std::endl;

	start = std::chrono::high_resolution_clock::now();
#endif

	//assign current compute commandBuffer
	VkCommandBuffer computeCommandBuffer = computeCommandBuffers[frameIndex];

	//create beginInfo
	VkCommandBufferBeginInfo computeBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	computeBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;//assuming it is only used once

	//begin recording
	vkBeginCommandBuffer(computeCommandBuffer, &computeBeginInfo);

	//Bind Pipeline, current commandBuffer and set the Bind Point
	vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.pipeline);

	//Bind the layout and descriptorset
	vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.pipelineLayout, 0, 1, &computePipeline.descriptorSets[frameIndex], 0, 0);

	PushData push = { Xres, Yres };
	//push Constants to compute shader
	vkCmdPushConstants(computeCommandBuffer, computePipeline.pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushData), &push);

	//dispatch shader
	//vkCmdDispatch(computeCommandBuffer, Xres, Yres, 1); //assuming Xres*Yres is not exceeding maxWorkgroups of device and dimensions are devisible by localgroups
	vkCmdDispatch(computeCommandBuffer, (Xres + 31) / 32, (Yres + 31) / 32, 1);


	//end recording
	vkEndCommandBuffer(computeCommandBuffer);	

#ifdef VULKAN_TIME_PROFILE	
	end = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	std::cout << "Time taken to record compute pipeline command buffer: \t" << duration << " microseconds" << std::endl;

	start = std::chrono::high_resolution_clock::now();
#endif

	VkSemaphore pComputeWait[2] = { renderFinishedSemaphoreForCompute[frameIndex], computeFinishedSemaphoreForCompute[(frameIndex-1)%FRAMES_IN_FLIGHT]};
	VkSemaphore pComputeSignals[2] = { computeFinishedSemaphoreForGraphics[frameIndex], computeFinishedSemaphoreForCompute[frameIndex]};
	

	// Submit compute pipeline command buffer
	VkPipelineStageFlags pComputeWaitMasks[2] = { VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
	VkSubmitInfo computeSubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	computeSubmitInfo.commandBufferCount = 1;
	computeSubmitInfo.pCommandBuffers = &computeCommandBuffers[frameIndex];
	computeSubmitInfo.waitSemaphoreCount = 2;
	computeSubmitInfo.pWaitSemaphores = pComputeWait;
	//VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	computeSubmitInfo.pWaitDstStageMask = pComputeWaitMasks;
	computeSubmitInfo.signalSemaphoreCount = 2;
	computeSubmitInfo.pSignalSemaphores = pComputeSignals;

	VKA(vkQueueSubmit(context->computeQueue.queue, 1, &computeSubmitInfo, 0));
	
#ifdef VULKAN_TIME_PROFILE	
	end = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	std::cout << "Time taken to submit ComputeQueue: \t\t\t" << duration << " microseconds" << std::endl;

	start = std::chrono::high_resolution_clock::now();
#endif

	VkSemaphore pRenderWaits[2] = { acquireSemaphores[frameIndex],  computeFinishedSemaphoreForGraphics[frameIndex] };
	VkSemaphore pRenderSignals[2] = { renderFinishedSemaphoreForPresent[frameIndex],  renderFinishedSemaphoreForCompute[frameIndex] };

	//Submit graphics pipeline
	VkPipelineStageFlags waitDstStageMask[2] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
	VkSubmitInfo graphicsSubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	graphicsSubmitInfo.commandBufferCount = 1;
	graphicsSubmitInfo.pCommandBuffers = &commandBuffers[frameIndex];
	graphicsSubmitInfo.waitSemaphoreCount = 2;
	graphicsSubmitInfo.pWaitSemaphores = pRenderWaits;
	graphicsSubmitInfo.pWaitDstStageMask = waitDstStageMask;
	graphicsSubmitInfo.signalSemaphoreCount = 2;
	graphicsSubmitInfo.pSignalSemaphores = pRenderSignals;

	VkSubmitInfo pSubmits[2] = { computeSubmitInfo, graphicsSubmitInfo };
	//std::cout << "pSubmit[0] waiting for: renderFinishedSemaphoreForCompute[" << frameIndex << "] and computeFinishedSemaphoreForCompute[" << (frameIndex - 1) % FRAMES_IN_FLIGHT << "]" << std::endl;
	//std::cout << "pSubmit[1] waiting for: acquireSemaphores[" << frameIndex << "] and computeFinishedSemaphoreForGraphics[" << frameIndex << "]" << std::endl;


	
	VKA(vkQueueSubmit(context->graphicsQueue.queue, 1, &graphicsSubmitInfo, fencesGraphics[frameIndex]));
	//VKA(vkQueueSubmit(context->graphicsQueue.queue, 2, pSubmits, fencesGraphics[frameIndex]));
#ifdef VULKAN_TIME_PROFILE	
	end = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	std::cout << "Time taken to submit GraphicsQueue: \t\t\t" << duration << " microseconds" << std::endl;

	start = std::chrono::high_resolution_clock::now();
#endif
	//Present swapchain
	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain.swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphoreForPresent[frameIndex];
	result = VK(vkQueuePresentKHR(context->graphicsQueue.queue, &presentInfo));
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		// Swapchain is out of date
		recreateSwapchain();
	}
	else {
		ASSERT_VULKAN(result);
	}

	//end of render loop
	frameIndex = (frameIndex + 1) % FRAMES_IN_FLIGHT;
#ifdef VULKAN_TIME_PROFILE
	end = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	std::cout << "Time taken to present Swapchain: \t\t\t" << duration << " microseconds" << std::endl;
	

	auto total_end = std::chrono::high_resolution_clock::now();
	auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(total_end - total_start).count();
	std::cout << "Total Time taken for Renderloop: \t\t\t" << total_duration << " microseconds" << std::endl;
#endif
}

void shutdownApplication() {
	VKA(vkDeviceWaitIdle(context->device));	


	destroyBuffer(context, &vertexBuffer);
	destroyBuffer(context, &indexBuffer);
	destroyBuffer(context, &initialCellStateBuffer);

	for (size_t i = 0; i < ARRAY_COUNT(cellStateBuffers); i++)
	{
		destroyBuffer(context, &cellStateBuffers[i]);
	}



	for(uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
		VK(vkDestroyFence(context->device, fencesCompute[i], 0));
		VK(vkDestroyFence(context->device, fencesGraphics[i], 0));
		VK(vkDestroySemaphore(context->device, acquireSemaphores[i], 0));
		//VK(vkDestroySemaphore(context->device, releaseSemaphores[i], 0));
		VK(vkDestroySemaphore(context->device, computeFinishedSemaphoreForCompute[i], 0));
		VK(vkDestroySemaphore(context->device, computeFinishedSemaphoreForGraphics[i], 0));
		VK(vkDestroySemaphore(context->device, renderFinishedSemaphoreForCompute[i], 0));
		VK(vkDestroySemaphore(context->device, renderFinishedSemaphoreForPresent[i], 0));
	}
	for(uint32_t i = 0; i < ARRAY_COUNT(commandPools); ++i) {
		VK(vkDestroyCommandPool(context->device, commandPools[i], 0));
		VK(vkDestroyCommandPool(context->device, computeCommandPools[i], 0));
	}
	VK(vkDestroyCommandPool(context->device, initialCellStateCommandBufferPool, 0));

	destroyGraphicsPipeline(context, &graphicsPipeline);
	destroyComputePipeline(context, &computePipeline);

	for (uint32_t i = 0; i < framebuffers.size(); ++i) {
		VK(vkDestroyFramebuffer(context->device, framebuffers[i], 0));
	}
	framebuffers.clear();
	destroyRenderpass(context, renderPass);
	destroySwapchain(context, &swapchain);
	VK(vkDestroySurfaceKHR(context->instance, surface, 0));
	exitVulkan(context);
}

int main() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		LOG_ERROR("Error initializing SDL: ", SDL_GetError());
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow("Vulkan Tutorial", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 720,720,/*1240, 720,*/ SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	if (!window) {
		LOG_ERROR("Error creating SDL window");
		return 1;
	}

	initApplication(window);
	double sum = 0.0;
	double amount = 0;
	double avg = 60.0f;

	/*VkSemaphore pSignals[2 * FRAMES_IN_FLIGHT];

	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
	{
		pSignals[2 * i + 0] = renderFinishedSemaphoreForCompute[i];
		pSignals[2 * i + 1] = computeFinishedSemaphoreForCompute[i];
	}*/
	{
		VkSemaphore pSignals[1] = { computeFinishedSemaphoreForCompute[FRAMES_IN_FLIGHT - 1] };

		VkSubmitInfo initSubmit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		initSubmit.signalSemaphoreCount = 1;//2*FRAMES_IN_FLIGHT;
		initSubmit.pSignalSemaphores = pSignals;

		VKA(vkQueueSubmit(context->computeQueue.queue, 1, &initSubmit, 0));
	}

	{
		VkSemaphore pSignals[FRAMES_IN_FLIGHT] = { };

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
		{
			pSignals[i] = renderFinishedSemaphoreForCompute[i];

		}

		VkSubmitInfo initSubmit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		initSubmit.signalSemaphoreCount = FRAMES_IN_FLIGHT;//2*FRAMES_IN_FLIGHT;
		initSubmit.pSignalSemaphores = pSignals;

		VKA(vkQueueSubmit(context->graphicsQueue.queue, 1, &initSubmit, 0));
	}
	while (handleMessage()) {
		auto startTime = std::chrono::high_resolution_clock::now();
		renderApplication();

		auto endTime = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
		double fps = 1000000.0 / duration;
		sum += fps;
		amount += 1;
		avg = sum / amount;
		std::cout << "avg FPS: " << avg << " | current FPS: " << fps << std::endl;
	}

	shutdownApplication();

	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
}