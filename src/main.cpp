#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "logger.h"
#include "vulkan_base/vulkan_base.h"

#include <random>
#include <chrono>
#include <iostream>


#include "Buildingblocks/Buildingblocks.h"


std::mt19937 randomEngine(std::random_device{}());
std::uniform_int_distribution<uint32_t> distribution(0, 1);


#define FRAMES_IN_FLIGHT 32
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
VulkanBuffer cellDataBuffers[FRAMES_IN_FLIGHT];
VulkanBuffer globalGpuDataBuffer;
VulkanBuffer globalGpuDataUniformBuffer;
VulkanBuffer preCalcBuffer;
VulkanBuffer preCalcKernelBuffer;
VulkanBuffer preCalcKernelUniformBuffer;

VulkanBuffer initialCellStateBuffer;
VkCommandBuffer initialCellStateCommandBuffer;
VkCommandPool initialCellStateCommandBufferPool;

VkDescriptorBufferInfo cellStateBufferInfos[FRAMES_IN_FLIGHT];
VkDescriptorBufferInfo cellDataBufferInfos[FRAMES_IN_FLIGHT];
VkDescriptorBufferInfo preCalcBufferInfo;
VkDescriptorBufferInfo preCalcKernelBufferInfo;
VkDescriptorBufferInfo preCalcKernelUniformBufferInfo;
VkDescriptorBufferInfo globalGpuDataBufferInfo;
VkDescriptorBufferInfo globalGpuDataUniformBufferInfo;


VulkanPipeline computePipeline;
VulkanPipeline preCalcPipeline;
VulkanPipeline preCalcKernelPipeline;

VkCommandBuffer computeCommandBuffers[FRAMES_IN_FLIGHT];
VkCommandPool computeCommandPools[FRAMES_IN_FLIGHT];


VkCommandBuffer preCalcCommandBuffer;
VkCommandPool preCalcCommandPool;
VkCommandBuffer preCalcKernelCommandBuffer;
VkCommandPool preCalcKernelCommandPool;


Buildingblocks* builder = new Buildingblocks();
InputData globalGpuData;


int sensorSetSize = 16;
int clusterSetSize = 3;
int minimumDist = 0;
int maximumDist = 6;
int maximumNeurons = 20;
int maximumSensors = 3;
int maximumClusters = 2;
int maximumSpec = 1;
bool squareio = false;
int minWishPercent = 0;
int maxWishPercent = 5000;
int minStayPercent = 0;
int maxStayPercent = 5000;

const uint32_t Xres = 32 * 32*1;
const uint32_t Yres = 32 * 32*1;
uint32_t cellStateBufferSize = 2 * Xres * Yres * sizeof(uint32_t);
uint32_t cellDataBufferSize = Xres * Yres * sizeof(cellData);
uint32_t initialData[2*Xres * Yres];

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
			initialData[(y * Xres + x)*2 + 0] = distribution(randomEngine) * 15 + distribution(randomEngine);
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

				//initialData[y * Xres + x] = distribution(randomEngine) * 10;
			}

			if (x == 0 || y == 0 || y == 1 || x == 1 ) {
				initialData[(y * Xres + x)*2 + 0] = 13;
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

			uploadDataToBuffer(context, &cellStateBuffers[i], initialData, cellStateBufferSize);
			//ToDo: uploadDataToBuffer(context, &cellStateBuffers[i], initialData, Xres* Yres * sizeof(cellData));
			//ToDo: uploadDataToBuffer(context, &cellStateBuffers[i], initialData, 1000000*sizeof(calcData));
			//ToDo: uploadDataToBuffer(context, &cellStateBuffers[i], initialData, sizeof(InputData));

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



//Todo: Buildingblocks importieren
void initGameLogic() {

}

//ToDo 
void preCalc() {

	{
		preCalcPipeline = createComputePipeline(context, "../shaders/preCalc_comp.spv", 0, 0, 1, 0, 0, 0, 0, 0, 1, &preCalcBufferInfo, 0, 0);


		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		createInfo.queueFamilyIndex = context->computeQueue.familyIndex;
		VKA(vkCreateCommandPool(context->device, &createInfo, 0, &preCalcCommandPool));


		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = preCalcCommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		VKA(vkAllocateCommandBuffers(context->device, &allocateInfo, &preCalcCommandBuffer));

		//assign current compute commandBuffer
		VkCommandBuffer computeCommandBuffer = preCalcCommandBuffer;

		//create beginInfo
		VkCommandBufferBeginInfo computeBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		computeBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;//assuming it is only used once

		//begin recording
		vkBeginCommandBuffer(computeCommandBuffer, &computeBeginInfo);

		//Bind Pipeline, current commandBuffer and set the Bind Point
		vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, preCalcPipeline.pipeline);

		//Bind the layout and descriptorset
		vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, preCalcPipeline.pipelineLayout, 0, 1, &preCalcPipeline.descriptorSets[0], 0, 0);


		//dispatch shader
		//vkCmdDispatch(computeCommandBuffer, Xres, Yres, 1); //assuming Xres*Yres is not exceeding maxWorkgroups of device and dimensions are devisible by localgroups
		vkCmdDispatch(computeCommandBuffer, (Xres + 31) / 32, (Yres + 31) / 32, 1);


		//end recording
		vkEndCommandBuffer(computeCommandBuffer);


		VKA(vkQueueWaitIdle(context->computeQueue.queue));

		VkSubmitInfo computeSubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		computeSubmitInfo.commandBufferCount = 1;
		computeSubmitInfo.pCommandBuffers = &preCalcCommandBuffer;
		computeSubmitInfo.waitSemaphoreCount = 0;
		computeSubmitInfo.pWaitSemaphores = 0;
		//VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		computeSubmitInfo.pWaitDstStageMask = 0;
		computeSubmitInfo.signalSemaphoreCount = 0;
		computeSubmitInfo.pSignalSemaphores = 0;

		VKA(vkQueueSubmit(context->computeQueue.queue, 1, &computeSubmitInfo, 0));
		VKA(vkQueueWaitIdle(context->computeQueue.queue));

	}



	LOG_INFO("a");
	{
		std::vector< VkDescriptorBufferInfo> singleBuffers = { preCalcBufferInfo, globalGpuDataBufferInfo, preCalcKernelBufferInfo };
		std::vector< VkDescriptorBufferInfo>  singleUniforms = {};

		preCalcKernelPipeline = createComputePipeline(context, "../shaders/preCalc_Kernel.spv", 0, 0, 1, 0, 0, 0, 0, 0, singleBuffers.size(), singleBuffers.data(), singleUniforms.size(), singleUniforms.data());


		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		createInfo.queueFamilyIndex = context->computeQueue.familyIndex;
		VKA(vkCreateCommandPool(context->device, &createInfo, 0, &preCalcKernelCommandPool));


		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = preCalcKernelCommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		VKA(vkAllocateCommandBuffers(context->device, &allocateInfo, &preCalcKernelCommandBuffer));

		//assign current compute commandBuffer
		VkCommandBuffer computeCommandBuffer = preCalcKernelCommandBuffer;

		//create beginInfo
		VkCommandBufferBeginInfo computeBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		computeBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;//assuming it is only used once

		//begin recording
		vkBeginCommandBuffer(computeCommandBuffer, &computeBeginInfo);

		//Bind Pipeline, current commandBuffer and set the Bind Point
		vkCmdBindPipeline(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, preCalcKernelPipeline.pipeline);

		//Bind the layout and descriptorset
		vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, preCalcKernelPipeline.pipelineLayout, 0, 1, &preCalcKernelPipeline.descriptorSets[0], 0, 0);


		//dispatch shader
		//vkCmdDispatch(computeCommandBuffer, Xres, Yres, 1); //assuming Xres*Yres is not exceeding maxWorkgroups of device and dimensions are devisible by localgroups
		vkCmdDispatch(computeCommandBuffer, (Xres + 31) / 32, (Yres + 31) / 32, 1);


		//end recording
		vkEndCommandBuffer(computeCommandBuffer);


		VKA(vkQueueWaitIdle(context->computeQueue.queue));

		VkSubmitInfo computeSubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		computeSubmitInfo.commandBufferCount = 1;
		computeSubmitInfo.pCommandBuffers = &preCalcKernelCommandBuffer;
		computeSubmitInfo.waitSemaphoreCount = 0;
		computeSubmitInfo.pWaitSemaphores = 0;
		//VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		computeSubmitInfo.pWaitDstStageMask = 0;
		computeSubmitInfo.signalSemaphoreCount = 0;
		computeSubmitInfo.pSignalSemaphores = 0;

		VKA(vkQueueSubmit(context->computeQueue.queue, 1, &computeSubmitInfo, 0));
		VKA(vkQueueWaitIdle(context->computeQueue.queue));

		uploadDataToBuffer(context, &preCalcKernelUniformBuffer, 0, 0, &preCalcKernelBuffer);
	}
}

void initComputationBuffers() {

	uint32_t offset = 0;
	for (uint32_t i = 0; i < ARRAY_COUNT(cellDataBuffers); i++) {
		cellDataBuffers[i].offset = offset;
		createBuffer(context, &cellDataBuffers[i], Xres * Yres * sizeof(cellData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); //VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		cellDataBufferInfos[i].buffer = cellDataBuffers[i].buffer;
		cellDataBufferInfos[i].offset = 0;
		cellDataBufferInfos[i].range = cellDataBufferSize;
		offset += cellDataBufferSize;
	}

	int numberOfPreCalcBuffers = 1;
	int numberOfPreCalcValues = 1000000;
	uint32_t preCalcBufferSize = numberOfPreCalcValues * sizeof(calcData);

	createBuffer(context, &preCalcBuffer, preCalcBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); //VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	preCalcBufferInfo.buffer = preCalcBuffer.buffer;
	preCalcBufferInfo.offset = 0;
	preCalcBufferInfo.range = preCalcBufferSize;


	int numberOfPreCalcKernelBuffers = 1;
	int numberOfPreCalcKernelValues = 10000;
	uint32_t preCalcKernelBufferSize = numberOfPreCalcKernelValues * sizeof(int[3]);

	createBuffer(context, &preCalcKernelBuffer, preCalcKernelBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	preCalcKernelBufferInfo.buffer = preCalcKernelBuffer.buffer;
	preCalcKernelBufferInfo.offset = 0;
	preCalcKernelBufferInfo.range = preCalcKernelBufferSize;

	createBuffer(context, &preCalcKernelUniformBuffer, preCalcKernelBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);// VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); //VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	preCalcKernelUniformBufferInfo.buffer = preCalcKernelUniformBuffer.buffer;
	preCalcKernelUniformBufferInfo.offset = 0;
	preCalcKernelUniformBufferInfo.range = preCalcKernelBufferSize;
	


	int numberOfInputBuffers = 1;
	int numberOfInputValues = 1;
	uint32_t globalDataBufferSize = numberOfInputValues * sizeof(InputData);

	createBuffer(context, &globalGpuDataBuffer, globalDataBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT); //VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	globalGpuDataBufferInfo.buffer = globalGpuDataBuffer.buffer;
	globalGpuDataBufferInfo.offset = 0;
	globalGpuDataBufferInfo.range = globalDataBufferSize;
	
	createBuffer(context, &globalGpuDataUniformBuffer, globalDataBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); //VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	globalGpuDataUniformBufferInfo.buffer = globalGpuDataUniformBuffer.buffer;
	globalGpuDataUniformBufferInfo.offset = 0;
	globalGpuDataUniformBufferInfo.range = globalDataBufferSize;

	bool aBool = true;
	do {

		builder->standard_start(

			sensorSetSize,		//SensorMenge --> wieviele Sensorgruppen erstellt werden // max 16 
			minimumDist,		//minimale Distanz 
			maximumDist,		//maximale Distanz 
			maximumNeurons,		//maxSensoren -> aus wieivielen IDs kann ein Sensor bestehen? 
			clusterSetSize,		//ClusterMenge -> wieviele interprationscluster werden erstellt? //max 6 
			maximumSensors,		//maxGruppen -> wieviele sensorarten pro InterpretationsCluster m�glich? 
			maximumSpec,		//speziesmenge 
			maximumClusters,		//maxWertigkeiten wieviele Interpretationscluster pro spezies m�glich? 
			1000,	//maxWish 
			3000,
			squareio);	//maxStay 

		aBool = false;
		for (size_t i = 0; i < builder->SpeziesSet[0]->Brain.size(); i++)
		{
			aBool = (builder->SpeziesSet[0]->Brain[i]->wertigkeit < 0) || aBool;
		}

	} while (aBool);// || builder->maxCombiSum < 400); 

	int AuraSquare = int(builder->square);

	for (size_t i = 0; i < builder->SensorSet.size(); i++)
	{
		if (random(0, 100) % 2 == 0) {
			//builder->SensorSet[i]->updateRange(1, 40, 4, 6); 
		}
		std::cout << i << ": \t FIRST:" << builder->SensorSet[i]->firstID << std::endl;
		std::cout << "  " << " \t SIZE: " << builder->SensorSet[i]->groesse << std::endl;
		std::cout << "  " << " \t MIN: " << builder->SensorSet[i]->minID << std::endl;
		std::cout << "  " << " \t MAX: " << builder->SensorSet[i]->maxID << std::endl;
	}


	std::vector<int> rulesDNA = builder->SpeziesSet[0]->DNA;
	std::vector<int> bodyDNA = builder->SensorGruppenRawData;

	std::vector<int> fullBodyDNA(96, 0);
	std::vector<int> sensorGruppenNummern(96, 0);
	std::vector<int> sensorGruppenAnzahl = {1,0,0,0,0,0,0};

	std::cout << "Regeln: " << rulesDNA[0] << ", " << rulesDNA[1] << ", " << rulesDNA[2] << std::endl;
	std::cout << bodyDNA.size() << std::endl;

	for (size_t i = 0; i < bodyDNA.size(); i++)
	{
		fullBodyDNA[i] = bodyDNA[i];
	}

	std::cout << "m�gliche summen: " << builder->maxCombiSum << std::endl;

	for (size_t i = 0; i < builder->SpeziesSet[0]->Brain.size(); i++)
	{
		sensorGruppenAnzahl[i] = builder->SpeziesSet[0]->Brain[i]->SensorGruppenIDs.size();
		std::cout << "Cluster " << i << " sensorgruppen: " << builder->SpeziesSet[0]->Brain[i]->SensorGruppenIDs.size() << std::endl;
	}

	globalGpuData.square = false;
	for (int i = 0; i < 3; ++i) {
		globalGpuData.RulesDNA[i] = rulesDNA[i];
	}
	for (int i = 0; i < ARRAY_COUNT(globalGpuData.BodyDNA); ++i) {
		globalGpuData.BodyDNA[i] = fullBodyDNA[i];
	}
	for (int i = 0; i < ARRAY_COUNT(globalGpuData.sensorGruppenAnzahl); ++i) {
		globalGpuData.sensorGruppenAnzahl[i] = sensorGruppenAnzahl[i];
	}

	if (builder->maxCombiSum > 100000) {
		builder->SpeziesSet[0]->updateWishAmount(minWishPercent * (Uint32(builder->maxCombiSum) / 10000), (maxWishPercent + 1) * (Uint32(builder->maxCombiSum) / 10000));
		builder->SpeziesSet[0]->updateStayAmount(minStayPercent * (Uint32(builder->maxCombiSum) / 10000), (maxStayPercent + 1) * (Uint32(builder->maxCombiSum) / 10000));
	}
	else {
		if (builder->maxCombiSum > 10000) {
			builder->SpeziesSet[0]->updateWishAmount((float(minWishPercent) / 100) * (Uint32(builder->maxCombiSum) / 100), (float(maxWishPercent + 1) / 100) * (Uint32(builder->maxCombiSum) / 100));
			builder->SpeziesSet[0]->updateStayAmount((float(minStayPercent) / 100) * (Uint32(builder->maxCombiSum) / 100), (float(maxStayPercent + 1) / 100) * (Uint32(builder->maxCombiSum) / 100));
		}
		else {
			builder->SpeziesSet[0]->updateWishAmount((float(minWishPercent) / 10000) * (int(builder->maxCombiSum)), (float(maxWishPercent + 1) / 10000) * (int(builder->maxCombiSum)));
			builder->SpeziesSet[0]->updateStayAmount((float(minStayPercent) / 10000) * (int(builder->maxCombiSum)), (float(maxStayPercent + 1) / 10000) * (int(builder->maxCombiSum)));
		}
	}
	std::cout << "wish: " << builder->SpeziesSet[0]->WishAmount << "(" << (100 * 1.0f / (builder->maxCombiSum / (1 + builder->SpeziesSet[0]->WishAmount))) << "%)" << std::endl;
	std::cout << "stay: " << builder->SpeziesSet[0]->StayAmount << "(" << (10000 * 1.0f / (builder->maxCombiSum / (1 + (builder->SpeziesSet[0]->StayAmount / 100)))) << "%)" << std::endl;

	globalGpuData.Wish = builder->SpeziesSet[0]->WishAmount;
	globalGpuData.Stay = builder->SpeziesSet[0]->StayAmount;


	for (size_t i = 0; i < builder->SpeziesSet[0]->Brain.size(); i++)
	{

		for (size_t j = 0; j < builder->SpeziesSet[0]->Brain[i]->SensorGruppenIDs.size(); j++)
		{
			sensorGruppenNummern[16 * i + j] = builder->SpeziesSet[0]->Brain[i]->SensorGruppenIDs[j];
		}
	}


	for (int i = 0; i < ARRAY_COUNT(globalGpuData.SensorGruppenIDs); ++i) {
		globalGpuData.SensorGruppenIDs[i] = sensorGruppenNummern[i];
	}
	
	globalGpuData.anzahlCluster = builder->SpeziesSet[0]->Brain.size();

	for (size_t i = 0; i < builder->SpeziesSet[0]->Brain.size(); i++)
	{
		globalGpuData.Wertigkeiten[i] = builder->SpeziesSet[0]->Brain[i]->wertigkeit;
		std::cout << "Cluster " << i << " Wert: " << globalGpuData.Wertigkeiten[i] << std::endl;
	}

	globalGpuData.maxCombiSum = builder->maxCombiSum;

	globalGpuData.Xres = Xres;
	globalGpuData.Yres = Yres;

	globalGpuData.thisSpecies = 1;
	
	globalGpuData.stateGroups = 10; // stateMod
	globalGpuData.sumTest = 1;
	globalGpuData.testint = 1;
	std::vector<int> DelayValues(18, 0);
	DelayValues[1] = 1;
	
	for (size_t i = 0; i < ARRAY_COUNT(globalGpuData.speciesRetardValues); i++)
	{
		globalGpuData.speciesRetardValues[i] = DelayValues[i];
	} 

	std::vector<int> ThreshValues(18, 0);
	ThreshValues[0] = 40000;
	ThreshValues[1] = 0;
	ThreshValues[2] = 60000;

	for (size_t i = 0; i < ARRAY_COUNT(globalGpuData.speciesThresh); i++)
	{
		globalGpuData.speciesThresh[i] = ThreshValues[i];
	}


	std::vector<int> speciesAcc(18, 0);
	speciesAcc[0] = -2000;
	speciesAcc[1] = 0;
	speciesAcc[2] = 2000;

	for (size_t i = 0; i < ARRAY_COUNT(globalGpuData.speciesAcc); i++)
	{
		globalGpuData.speciesAcc[i] = speciesAcc[i];
	}
	
	globalGpuData.factorPart = 0.0f;


	uploadDataToBuffer(context, &globalGpuDataBuffer, (void*)&globalGpuData, sizeof(InputData)); // works
	uploadDataToBuffer(context, &globalGpuDataUniformBuffer, (void*)&globalGpuData, sizeof(InputData)); // does not work

	LOG_INFO("wish:", globalGpuData.Wish);

	//uploadDataToBuffer(context, &globalGpuDataUniformBuffer, 0, 0, &globalGpuDataBuffer);
	//uploadDataToBuffer(context, &globalGpuDataUniformBuffer, (void*)&globalGpuData, sizeof(InputData), &globalGpuDataBuffer);
	preCalc();

	std::cout << "what" << std::endl;
}

void initCompute() {

	initComputationBuffers();
	
	//ToDo: an bufferinfos die neuen Buffer anh�ngen. DescriptorSets bleibt also gleich aber DescriptorSetSize wird erweitert. 
	//ToDo: 
	std::vector< VkDescriptorBufferInfo> singleBuffers = { preCalcBufferInfo, /*globalGpuDataBufferInfo*/};
	std::vector< VkDescriptorBufferInfo>  singleUniforms = {  preCalcKernelUniformBufferInfo, globalGpuDataUniformBufferInfo };

	computePipeline = createComputePipeline(context, "../shaders/cellSimulation_comp.spv", cellStateBufferInfos, cellDataBufferInfos, FRAMES_IN_FLIGHT, computeDescriptorSetSize,2, sizeof(PushData), 0, 0, singleBuffers.size(), singleBuffers.data(), singleUniforms.size(), singleUniforms.data());

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

	const char* enabledDeviceExtensions[]{ VK_KHR_SWAPCHAIN_EXTENSION_NAME  };
	context = initVulkan(instanceExtensionCount, enabledInstanceExtensions, ARRAY_COUNT(enabledDeviceExtensions), enabledDeviceExtensions);

	SDL_Vulkan_CreateSurface(window, context->instance, &surface);
	swapchain = createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

	recreateRenderPass();

	//create internal buffers
	uint32_t offset = 0;
	for (uint32_t i = 0; i < ARRAY_COUNT(cellStateBuffers); i++) {
		cellStateBuffers[i].offset = offset;
		createBuffer(context, &cellStateBuffers[i], cellStateBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); //VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		cellStateBufferInfos[i].buffer = cellStateBuffers[i].buffer;
		cellStateBufferInfos[i].offset = 0;
		cellStateBufferInfos[i].range = cellStateBufferSize;
		offset += cellStateBufferSize;
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
	graphicsPipeline = createGraphicsPipeline(context, "../shaders/readBuffer_vert.spv", "../shaders/readBuffer_frag.spv", renderPass, swapchain.width, swapchain.height, vertexAttributeDescriptions, ARRAY_COUNT(vertexAttributeDescriptions), &vertexInputBinding, computeDescriptorSetSize, FRAMES_IN_FLIGHT, sizeof(PushData), cellStateBufferInfos);

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

	VkDescriptorSet a[2] = { computePipeline.descriptorSets[frameIndex], computePipeline.uniformsDescriptorSets[frameIndex] };

	//Bind the layout and descriptorset
	vkCmdBindDescriptorSets(computeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline.pipelineLayout, 0, 2, a, 0, 0);

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

	/*

	*/
	destroyBuffer(context, &vertexBuffer);
	destroyBuffer(context, &indexBuffer);
	destroyBuffer(context, &initialCellStateBuffer);
	destroyBuffer(context, &globalGpuDataBuffer);
	destroyBuffer(context, &globalGpuDataUniformBuffer);
	destroyBuffer(context, &preCalcBuffer);
	destroyBuffer(context, &preCalcKernelBuffer);
	destroyBuffer(context, &preCalcKernelUniformBuffer);

	for (size_t i = 0; i < ARRAY_COUNT(cellStateBuffers); i++)
	{
		destroyBuffer(context, &cellStateBuffers[i]);
		destroyBuffer(context, &cellDataBuffers[i]);
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
	VK(vkDestroyCommandPool(context->device, preCalcCommandPool, 0));
	VK(vkDestroyCommandPool(context->device, preCalcKernelCommandPool, 0));

	destroyGraphicsPipeline(context, &graphicsPipeline);
	destroyComputePipeline(context, &computePipeline);
	destroyComputePipeline(context, &preCalcPipeline);
	destroyComputePipeline(context, &preCalcKernelPipeline);

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



/*
do {
						builder->standard_start(
							sensorSetSize,		//SensorMenge --> wieviele Sensorgruppen erstellt werden // max 16
							minimumDist,		//minimale Distanz
							maximumDist,		//maximale Distanz
							maximumNeurons,		//maxSensoren -> aus wieivielen IDs kann ein Sensor bestehen?
							clusterSetSize,		//ClusterMenge -> wieviele interprationscluster werden erstellt? //max 6
							maximumSensors,		//maxGruppen -> wieviele sensorarten pro InterpretationsCluster m�glich?
							maximumSpec,		//speziesmenge
							maximumClusters,		//maxWertigkeiten wieviele Interpretationscluster pro spezies m�glich?
							1000,	//maxWish
							3000,
							squareio);	//maxStay

						aBool = false;
						for (size_t i = 0; i < builder->SpeziesSet[0]->Brain.size(); i++)
						{
							aBool = (builder->SpeziesSet[0]->Brain[i]->wertigkeit < 0) || aBool;
						}

					} while (aBool);// || builder->maxCombiSum < 400);
*/