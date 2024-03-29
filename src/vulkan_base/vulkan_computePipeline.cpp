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
    VkDescriptorBufferInfo* extraSingleUniformBufferInfos) {

    VkPipeline pipeline;


    // Set up the descriptor pool size
    VkDescriptorPoolSize uniformPoolSize;
    uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformPoolSize.descriptorCount = (amountExtraSingleUniformsPerSet)*numDescriptorSets;


    // Create the descriptor pool create info
    VkDescriptorPoolCreateInfo uniformsPoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    uniformsPoolInfo.poolSizeCount = 1;
    uniformsPoolInfo.pPoolSizes = &uniformPoolSize;
    uniformsPoolInfo.maxSets = numDescriptorSets;


    VkDescriptorPool uniformsDescriptorPool;
    // Create the descriptor pool

    if (amountExtraSingleUniformsPerSet != 0) {
        if (vkCreateDescriptorPool(context->device, &uniformsPoolInfo, nullptr, &uniformsDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    // define the Layout and create Info
    VkDescriptorSetLayoutCreateInfo uniformsLayoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    {
        VkDescriptorSetLayoutBinding* uniformsLayoutBindings = new VkDescriptorSetLayoutBinding[(amountExtraSingleUniformsPerSet)];
        for (size_t i = 0; i < (amountExtraSingleUniformsPerSet); i++)
        {
            uniformsLayoutBindings[i].binding = i;
            uniformsLayoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uniformsLayoutBindings[i].descriptorCount = 1;
            uniformsLayoutBindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        }
        uniformsLayoutCreateInfo.bindingCount = (amountExtraSingleUniformsPerSet);
        uniformsLayoutCreateInfo.pBindings = uniformsLayoutBindings;
    }


    //create single allocation Info
    VkDescriptorSetLayout uniformsDescriptorSetLayout;
    VkDescriptorSetAllocateInfo uniformsDescriptorSetAllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };

    VkDescriptorSetLayout uniformsLayout;

    //allocate Sets based on Info
    VkDescriptorSet* uniformsDescriptorSets = new VkDescriptorSet[numDescriptorSets];

    if (amountExtraSingleUniformsPerSet != 0) {

        vkCreateDescriptorSetLayout(context->device, &uniformsLayoutCreateInfo, 0, &uniformsDescriptorSetLayout);

        uniformsLayout = uniformsDescriptorSetLayout;

        std::vector<VkDescriptorSetLayout> uniformsLayouts(numDescriptorSets, uniformsLayout);
        uniformsDescriptorSetAllocateInfo.descriptorPool = uniformsDescriptorPool;
        uniformsDescriptorSetAllocateInfo.descriptorSetCount = numDescriptorSets;
        uniformsDescriptorSetAllocateInfo.pSetLayouts = uniformsLayouts.data();

        if (vkAllocateDescriptorSets(context->device, &uniformsDescriptorSetAllocateInfo, uniformsDescriptorSets) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }


        std::vector<VkWriteDescriptorSet> writeDescriptorSets(numDescriptorSets * (amountExtraSingleUniformsPerSet));


        //ToDo: nicht alle brauchen einen Swap-buffer. Swaps ist nicht immer descriptorSetSize. meistens eigentlich 2
        for (uint32_t i = 0; i < numDescriptorSets; i++)
        {
            uint32_t uniformsDescriptorSetSizeTotal = (amountExtraSingleUniformsPerSet);

            for (size_t j = 0; j < amountExtraSingleUniformsPerSet; j++)
            {
                writeDescriptorSets[uniformsDescriptorSetSizeTotal * i + j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptorSets[uniformsDescriptorSetSizeTotal * i + j].dstSet = uniformsDescriptorSets[i];
                writeDescriptorSets[uniformsDescriptorSetSizeTotal * i + j].dstBinding = j;
                writeDescriptorSets[uniformsDescriptorSetSizeTotal * i + j].dstArrayElement = 0;
                writeDescriptorSets[uniformsDescriptorSetSizeTotal * i + j].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                writeDescriptorSets[uniformsDescriptorSetSizeTotal * i + j].descriptorCount = 1;
                writeDescriptorSets[uniformsDescriptorSetSizeTotal * i + j].pBufferInfo = &extraSingleUniformBufferInfos[j];
            }
        }


        //update Sets
        vkUpdateDescriptorSets(context->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);

    }







    // Set up the descriptor pool size
    VkDescriptorPoolSize poolSize;
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = (descriptorSetSize_1 + descriptorSetSize_2 + amountExtraSingleStoragesPerSet) * numDescriptorSets;


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
        VkDescriptorSetLayoutBinding* layoutBindings = new VkDescriptorSetLayoutBinding[(descriptorSetSize_1 + descriptorSetSize_2 + amountExtraSingleStoragesPerSet)];
        for (size_t i = 0; i < (descriptorSetSize_1 + descriptorSetSize_2 + amountExtraSingleStoragesPerSet); i++)
        {
            layoutBindings[i].binding = i;
            layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            layoutBindings[i].descriptorCount = 1;
            layoutBindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        }

        layoutCreateInfo.bindingCount = (descriptorSetSize_1 + descriptorSetSize_2 + amountExtraSingleStoragesPerSet);
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
    std::vector<VkWriteDescriptorSet> writeDescriptorSets(numDescriptorSets * (descriptorSetSize_1 + descriptorSetSize_2 + amountExtraSingleStoragesPerSet));

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


    //ToDo: nicht alle brauchen einen Swap-buffer. Swaps ist nicht immer descriptorSetSize. meistens eigentlich 2
    for (uint32_t i = 0; i < numDescriptorSets; i++)
    {
        uint32_t descriptorSetSizeTotal = (descriptorSetSize_1 + descriptorSetSize_2 + amountExtraSingleStoragesPerSet);

        for (uint32_t j = 0; j < descriptorSetSize_1; j++)
        {
            writeDescriptorSets[descriptorSetSizeTotal * i + j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[descriptorSetSizeTotal * i + j].dstSet = descriptorSets[i];
            writeDescriptorSets[descriptorSetSizeTotal * i + j].dstBinding = j;
            writeDescriptorSets[descriptorSetSizeTotal * i + j].dstArrayElement = 0;
            writeDescriptorSets[descriptorSetSizeTotal * i + j].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writeDescriptorSets[descriptorSetSizeTotal * i + j].descriptorCount = 1;
            writeDescriptorSets[descriptorSetSizeTotal * i + j].pBufferInfo = &bufferInfos_1[(i + j) % numDescriptorSets];
        }

        uint32_t assignedDescriptors = descriptorSetSizeTotal * i + descriptorSetSize_1;

        for (uint32_t j = 0; j < descriptorSetSize_2; j++)
        {
            writeDescriptorSets[assignedDescriptors + j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[assignedDescriptors + j].dstSet = descriptorSets[i];
            writeDescriptorSets[assignedDescriptors + j].dstBinding = descriptorSetSize_1 + j;
            writeDescriptorSets[assignedDescriptors + j].dstArrayElement = 0;
            writeDescriptorSets[assignedDescriptors + j].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writeDescriptorSets[assignedDescriptors + j].descriptorCount = 1;
            writeDescriptorSets[assignedDescriptors + j].pBufferInfo = &bufferInfos_2[(i + j) % numDescriptorSets];
        }

        assignedDescriptors += descriptorSetSize_2;

        for (size_t j = 0; j < amountExtraSingleStoragesPerSet; j++)
        {
            writeDescriptorSets[assignedDescriptors + j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[assignedDescriptors + j].dstSet = descriptorSets[i];
            writeDescriptorSets[assignedDescriptors + j].dstBinding = descriptorSetSize_1 + descriptorSetSize_2 + j;
            writeDescriptorSets[assignedDescriptors + j].dstArrayElement = 0;
            writeDescriptorSets[assignedDescriptors + j].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writeDescriptorSets[assignedDescriptors + j].descriptorCount = 1;
            writeDescriptorSets[assignedDescriptors + j].pBufferInfo = &extraSingleBufferInfos[j];
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

    if (amountExtraSingleUniformsPerSet != 0) {
        VkDescriptorSetLayout sets[2] = { descriptorSetLayout , uniformsDescriptorSetLayout };
        pipelineLayoutCreateInfo.setLayoutCount = 2;
        pipelineLayoutCreateInfo.pSetLayouts = sets;
    }
    else {
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    }

    if (pushDataSize > 0) {
        VkPushConstantRange pushConstantRange = {};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = pushDataSize;

        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    }

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
    computePipeline.uniformsDescriptorSets = NULL;
    computePipeline.uniformsDescriptorPool = NULL;
    computePipeline.uniformsDescriptorSetLayout = NULL;

    if (amountExtraSingleUniformsPerSet != 0) {
        computePipeline.uniformsDescriptorSets = uniformsDescriptorSets;
        computePipeline.uniformsDescriptorPool = uniformsDescriptorPool;
        computePipeline.uniformsDescriptorSetLayout = uniformsDescriptorSetLayout;
        computePipeline.uniformsLayoutCreateInfo = uniformsLayoutCreateInfo;
    }

    return computePipeline;
}

void destroyComputePipeline(VulkanContext* context, VulkanPipeline* pipeline) {
    VK(vkDestroyPipeline(context->device, pipeline->pipeline, 0));
    VK(vkDestroyPipelineLayout(context->device, pipeline->pipelineLayout, 0));
    VK(vkDestroyDescriptorPool(context->device, pipeline->descriptorPool, 0));
    VK(vkDestroyDescriptorSetLayout(context->device, pipeline->descriptorSetLayout, 0));
    
    if (pipeline->uniformsDescriptorPool != NULL) {
        VK(vkDestroyDescriptorPool(context->device, pipeline->uniformsDescriptorPool, 0));
        VK(vkDestroyDescriptorSetLayout(context->device, pipeline->uniformsDescriptorSetLayout, 0));
        delete[] pipeline->uniformsDescriptorSets;

    }
    

    delete[] pipeline->descriptorSets;
    delete[] pipeline->layoutCreateInfo.pBindings;
}



