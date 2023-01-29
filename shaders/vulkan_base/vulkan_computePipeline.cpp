#include "vulkan_base.h"

VkShaderModule createShaderModule2
(VulkanContext* context, const char* shaderFilename) {
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


VulkanPipeline createComputePipeline(VulkanContext* context,
    const char* computeShaderFilename,
    VkDescriptorBufferInfo* bufferInfos,
    uint32_t numDescriptorSets,
    uint32_t descriptorSetSize,
    uint32_t pushDataSize,
    VkPipelineCreateFlags flags,
    VkSpecializationInfo* specializationInfo) {

    VkPipeline pipeline;

    

    // Set up the descriptor pool size
    VkDescriptorPoolSize poolSize;
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = descriptorSetSize * numDescriptorSets;


    // Create the descriptor pool create info
    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = numDescriptorSets;

    
       

    
    
    VkDescriptorPool descriptorPool;
    // Create the descriptor pool
    if (vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    // define the Layout and create Info
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    {
        VkDescriptorSetLayoutBinding* layoutBindings = new VkDescriptorSetLayoutBinding[descriptorSetSize];
        for (size_t i = 0; i < descriptorSetSize; i++)
        {
            layoutBindings[i].binding = i;
            layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            layoutBindings[i].descriptorCount = 1;
            layoutBindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        }

        layoutCreateInfo.bindingCount = descriptorSetSize;
        layoutCreateInfo.pBindings = layoutBindings;

    }

    //create single allocation Info
    VkDescriptorSetLayout descriptorSetLayout;    
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    
    vkCreateDescriptorSetLayout(context->device, &layoutCreateInfo, 0, &descriptorSetLayout);
    VkDescriptorSetLayout layout = descriptorSetLayout;
    std::vector<VkDescriptorSetLayout> layouts(numDescriptorSets, layout);
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = numDescriptorSets;
    descriptorSetAllocateInfo.pSetLayouts = layouts.data();    
    

    //allocate Sets based on Info
    VkDescriptorSet* descriptorSets = new VkDescriptorSet[numDescriptorSets];
    if (vkAllocateDescriptorSets(context->device, &descriptorSetAllocateInfo, descriptorSets) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    } 

    //VkWriteDescriptorSet* writeDescriptorSets = new VkWriteDescriptorSet[numDescriptorSets * descriptorSetSize];
    std::vector<VkWriteDescriptorSet> writeDescriptorSets(numDescriptorSets * descriptorSetSize);

    /*
    for (size_t i = 0; i < numDescriptorSets * descriptorSetSize; i++)
    {
        writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[i].dstSet = descriptorSets[i / descriptorSetSize];
        writeDescriptorSets[i].dstBinding = i % descriptorSetSize;
        writeDescriptorSets[i].dstArrayElement = 0;
        writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writeDescriptorSets[i].descriptorCount = 1;
        writeDescriptorSets[i].pBufferInfo = &bufferInfos[i];
    }*/
    LOG_INFO("A");
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

    // Create the compute shader module
    VkShaderModule computeShaderModule = createShaderModule2(context, computeShaderFilename);

    // Create the compute pipeline stage
    VkPipelineShaderStageCreateInfo computeShaderStage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    computeShaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStage.module = computeShaderModule;
    computeShaderStage.pName = "main";
    computeShaderStage.pSpecializationInfo = specializationInfo;


    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = pushDataSize;

    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(context->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);

    // Create the compute pipeline create info
    VkComputePipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    pipelineCreateInfo.stage = computeShaderStage;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.flags = flags;

    // Create the compute pipeline
    VKA(vkCreateComputePipelines(context->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));

    
    VK(vkDestroyShaderModule(context->device, computeShaderModule, 0));

    VulkanPipeline computePipeline;
    computePipeline.pipeline = pipeline;
    computePipeline.pipelineLayout = pipelineLayout;
    computePipeline.descriptorSets = descriptorSets;
    computePipeline.descriptorSetLayout = descriptorSetLayout;
    computePipeline.descriptorPool = descriptorPool;
    computePipeline.layoutCreateInfo = layoutCreateInfo;

    return computePipeline;
}

void destroyComputePipeline(VulkanContext* context, VulkanPipeline* pipeline) {
    VK(vkDestroyPipeline(context->device, pipeline->pipeline, 0));
    VK(vkDestroyPipelineLayout(context->device, pipeline->pipelineLayout, 0));
    VK(vkDestroyDescriptorPool(context->device, pipeline->descriptorPool, 0));
    VK(vkDestroyDescriptorSetLayout(context->device, pipeline->descriptorSetLayout, 0));
    
    delete[] pipeline->descriptorSets;
    delete[] pipeline->layoutCreateInfo.pBindings;
}



