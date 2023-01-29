#include "vulkan_base.h"

VkShaderModule createShaderModule(VulkanContext* context, const char* shaderFilename) {
	VkShaderModule result = {};

	// Read shader file
	FILE* file = fopen(shaderFilename, "rb");
	if (!file) {
		LOG_ERROR("Shader not found: ", shaderFilename);
		return result;
	}
	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	assert((fileSize & 0x03) == 0);
	uint8_t* buffer = new uint8_t[fileSize];
	fread(buffer, 1, fileSize, file);

	VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = fileSize;
	createInfo.pCode = (uint32_t*)buffer;
	VKA(vkCreateShaderModule(context->device, &createInfo, 0, &result));

	delete[] buffer;
	fclose(file);

	return result;
}

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
	VkDescriptorBufferInfo* bufferInfos) {


	uint32_t descriptorSetSize = numReadComputeStorages;
	VkShaderModule vertexShaderModule = createShaderModule(context, vertexShaderFilename);
	VkShaderModule fragmentShaderModule = createShaderModule(context, fragmentShaderFilename);

	VkPipelineShaderStageCreateInfo shaderStages[2];
	shaderStages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vertexShaderModule;
	shaderStages[0].pName = "main";
	shaderStages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = fragmentShaderModule;
	shaderStages[1].pName = "main";

	VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputState.vertexBindingDescriptionCount = binding ? 1 : 0;
	vertexInputState.pVertexBindingDescriptions = binding;
	vertexInputState.vertexAttributeDescriptionCount = numAttributes;
	vertexInputState.pVertexAttributeDescriptions = attributes;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	//VkViewport viewport = { 0.0f, 0.0f, (float)width, (float)height };
	//viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	//VkRect2D scissor = { {0, 0}, {width, height} };
	//viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationState.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorBlendAttachment;

	VkPipelineDynamicStateCreateInfo dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	dynamicState.dynamicStateCount = ARRAY_COUNT(dynamicStates);
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineLayout pipelineLayout;

	VkDescriptorSet* descriptorSets = new VkDescriptorSet[numDescriptorSets];

	VkDescriptorSetLayout descriptorSetLayout;

	VkDescriptorPool descriptorPool;

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	{
		// Set up the descriptor pool size
		VkDescriptorPoolSize poolSize;
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSize.descriptorCount = descriptorSetSize * numDescriptorSets;


		// Create the descriptor pool create info
		VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = numDescriptorSets;

		// Create the descriptor pool
		if (vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}

		// define the Layout and create Info
		{
			VkDescriptorSetLayoutBinding* layoutBindings = new VkDescriptorSetLayoutBinding[descriptorSetSize];
			for (size_t i = 0; i < descriptorSetSize; i++)
			{
				layoutBindings[i].binding = i;
				layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				layoutBindings[i].descriptorCount = 1;
				layoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			}

			layoutCreateInfo.bindingCount = descriptorSetSize;
			layoutCreateInfo.pBindings = layoutBindings;
		}

		//create single allocation Info
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };

		vkCreateDescriptorSetLayout(context->device, &layoutCreateInfo, 0, &descriptorSetLayout);
		VkDescriptorSetLayout layout = descriptorSetLayout;
		std::vector<VkDescriptorSetLayout> layouts(numDescriptorSets, layout);
		descriptorSetAllocateInfo.descriptorPool = descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = numDescriptorSets;
		descriptorSetAllocateInfo.pSetLayouts = layouts.data();

		//allocate Sets based on Info
		if (vkAllocateDescriptorSets(context->device, &descriptorSetAllocateInfo, descriptorSets) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		//VkWriteDescriptorSet* writeDescriptorSets = new VkWriteDescriptorSet[numDescriptorSets * descriptorSetSize];
		std::vector<VkWriteDescriptorSet> writeDescriptorSets(numDescriptorSets * descriptorSetSize);

		/*for (size_t i = 0; i < numDescriptorSets * descriptorSetSize; i++)
		{
			writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[i].dstSet = descriptorSets[i / descriptorSetSize];
			writeDescriptorSets[i].dstBinding = i % descriptorSetSize;
			writeDescriptorSets[i].dstArrayElement = 0;
			writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptorSets[i].descriptorCount = 1;
			writeDescriptorSets[i].pBufferInfo = &bufferInfos[i];
		}*/

		for (uint32_t i = 0; i < numDescriptorSets; i++)
		{
			for (uint32_t j = 0; j < descriptorSetSize; j++)
			{
				writeDescriptorSets[descriptorSetSize * i + j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSets[descriptorSetSize * i + j].dstSet = descriptorSets[i];
				writeDescriptorSets[descriptorSetSize * i + j].dstBinding = j;
				writeDescriptorSets[descriptorSetSize * i + j].dstArrayElement = 0;
				writeDescriptorSets[descriptorSetSize * i + j].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				writeDescriptorSets[descriptorSetSize * i + j].descriptorCount = 1;
				writeDescriptorSets[descriptorSetSize * i + j].pBufferInfo = &bufferInfos[(i + j) % numDescriptorSets];
			}
		}

		//update Sets
		vkUpdateDescriptorSets(context->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);


		//Bind the descriptor set to the pipeline at pipeline creation time, by passing it in VkPipelineLayoutCreateInfo in the pSetLayouts field.
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = pushDataSize;

		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

		VKA(vkCreatePipelineLayout(context->device, &pipelineLayoutInfo, nullptr, &pipelineLayout));
	}

	VkPipeline pipeline;
	{
		VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		createInfo.stageCount = ARRAY_COUNT(shaderStages);
		createInfo.pStages = shaderStages;
		createInfo.pVertexInputState = &vertexInputState;
		createInfo.pInputAssemblyState = &inputAssemblyState;
		createInfo.pViewportState = &viewportState;
		createInfo.pRasterizationState = &rasterizationState;
		createInfo.pMultisampleState = &multisampleState;
		createInfo.pColorBlendState = &colorBlendState;
		createInfo.pDynamicState = &dynamicState;
		createInfo.layout = pipelineLayout;
		createInfo.renderPass = renderPass;
		createInfo.subpass = 0;
		VKA(vkCreateGraphicsPipelines(context->device, 0, 1, &createInfo, 0, &pipeline));

	}

	// Module can be destroyed after pipeline creation
	VK(vkDestroyShaderModule(context->device, vertexShaderModule, 0));
	VK(vkDestroyShaderModule(context->device, fragmentShaderModule, 0));

	VulkanPipeline result = {};
	result.pipeline = pipeline;
	result.pipelineLayout = pipelineLayout;
	result.descriptorSets = descriptorSets;
	result.descriptorSetLayout = descriptorSetLayout;
	result.descriptorPool = descriptorPool;
	result.layoutCreateInfo = layoutCreateInfo;

	return result;
}

void destroyGraphicsPipeline(VulkanContext* context, VulkanPipeline* pipeline) {
	VK(vkDestroyPipeline(context->device, pipeline->pipeline, 0));
	VK(vkDestroyPipelineLayout(context->device, pipeline->pipelineLayout, 0));
	VK(vkDestroyDescriptorPool(context->device, pipeline->descriptorPool, 0));
	VK(vkDestroyDescriptorSetLayout(context->device, pipeline->descriptorSetLayout, 0));

	delete[] pipeline->descriptorSets;
	delete[] pipeline->layoutCreateInfo.pBindings;
}