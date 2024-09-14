#include "pipeline.hpp"

#include <fstream>

using std::string, std::vector;

VkVertexInputBindingDescription Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    return attributeDescriptions;
}

bool Vertex::operator==(const Vertex &other) const {
    return pos == other.pos && color == other.color;
}

static vector<char> readFile(const string& filepath) {
    vector<char> buffer;

    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader spv file! Filepath: " + filepath);
    }

    long fileSize = file.tellg();
    buffer.resize(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

static VkShaderModule createShaderModule(VkDevice device, const vector<char>& shadercode) {
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = shadercode.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shadercode.data());

    VkShaderModule shaderModule;
    VkResult shader_module_creation_result = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);
    if (shader_module_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create pipeline shader module!", shader_module_creation_result);
    }
    return shaderModule;
}


static VkShaderModule loadShader(VkDevice device, const string& filename) {
    vector<char> shaderContent = readFile(SHADER_FOLDER_PATH + filename + SHADER_EXTENSION);
    return createShaderModule(device, shaderContent);
}

void GraphicsPipeline::create() {
    VkShaderModule vertexShader = loadShader(_device, _vertshadername);
    VkShaderModule fragmentShader = loadShader(_device, _fragshadername);

    VkShaderModule vertexParticleShader = loadShader(_device, _vertparticleshadername);
    VkShaderModule fragmentParticleShader = loadShader(_device, _fragparticleshadername);

    initRenderPass();
    initDescriptorSetLayout();
    initLayout();
    initPipeline(vertexShader, fragmentShader, vertexParticleShader, fragmentParticleShader);

    vkDestroyShaderModule(_device, vertexShader, nullptr);
    vkDestroyShaderModule(_device, fragmentShader, nullptr);
    vkDestroyShaderModule(_device, vertexParticleShader, nullptr);
    vkDestroyShaderModule(_device, fragmentParticleShader, nullptr);
}

void GraphicsPipeline::initRenderPass() {
    // Color Attachments
    VkAttachmentDescription colorAttachmentDescription = {};
    colorAttachmentDescription.format = _format;
    colorAttachmentDescription.samples = _msaasamples;

    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentReference.attachment = 0;

    VkAttachmentDescription colorAttachmentResolveDescription{};
    colorAttachmentResolveDescription.format = _format;
    colorAttachmentResolveDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolveDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolveDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolveDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolveDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolveDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolveDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentResolveReference{};
    colorAttachmentResolveReference.attachment = 2;
    colorAttachmentResolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth Attachment
    VkAttachmentDescription depthAttachmentDescription = {};
    depthAttachmentDescription.format = _depthformat;
    depthAttachmentDescription.samples = _msaasamples;
    depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Subpass
    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;
    subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
    subpassDescription.pResolveAttachments = &colorAttachmentResolveReference;

    VkSubpassDependency subpassDependency = {};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Renderpass
    std::array<VkAttachmentDescription, 3> renderPassAttachments = {colorAttachmentDescription, depthAttachmentDescription,
                                                                    colorAttachmentResolveDescription};
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<int32_t>(renderPassAttachments.size());
    renderPassCreateInfo.pAttachments = renderPassAttachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &subpassDependency;

    VkResult render_pass_creation_result = vkCreateRenderPass(_device, &renderPassCreateInfo, nullptr, &renderpass);
    if (render_pass_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create pipeline render pass, VkResult: %i\n", render_pass_creation_result);
    }
}

void GraphicsPipeline::initDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 2> uboLayoutBindings = {};
    uboLayoutBindings[0].binding = 0;
    uboLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBindings[0].descriptorCount = 1;
    uboLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    uboLayoutBindings[1].binding = 1;
    uboLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBindings[1].descriptorCount = 1;
    uboLayoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(uboLayoutBindings.size());
    layoutCreateInfo.pBindings = uboLayoutBindings.data();

    VkResult set_descriptor_creation_result = vkCreateDescriptorSetLayout(_device, &layoutCreateInfo, nullptr, &descriptorsetlayout);
    if (set_descriptor_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create descriptor setVertices layout!", set_descriptor_creation_result);
    }
}

void GraphicsPipeline::initLayout() {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorsetlayout;

    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    VkResult layout_creation_result = vkCreatePipelineLayout(_device, &pipelineLayoutCreateInfo, nullptr, &layout);
    if (layout_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create graphics pipeline layout!", layout_creation_result);
    }
}

void GraphicsPipeline::initPipeline(VkShaderModule vertexShader, VkShaderModule fragmentShader, VkShaderModule particleVertexShader, VkShaderModule particleFragmentShader) {
    // Shader Modules
    VkPipelineShaderStageCreateInfo vertexShaderStageInfo = {};
    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = vertexShader;
    vertexShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderStageInfo = {};
    fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderStageInfo.module = fragmentShader;
    fragmentShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageInfo, fragmentShaderStageInfo};

    // Vertex Input
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

    auto vertexBindingDescription = Vertex::getBindingDescription();
    auto vertexAttributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
    vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

    // Input Assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerCreateInfo.lineWidth = 1.0f;
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
    multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingCreateInfo.sampleShadingEnable = VK_TRUE;
    multisamplingCreateInfo.rasterizationSamples = _msaasamples;
    multisamplingCreateInfo.minSampleShading = 0.5f;
    multisamplingCreateInfo.pSampleMask = nullptr;
    multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisamplingCreateInfo.alphaToOneEnable = VK_FALSE;

    // Depth Stencil
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilCreateInfo.minDepthBounds = 0.0f;
    depthStencilCreateInfo.maxDepthBounds = 0.0f;
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
    depthStencilCreateInfo.front = {};
    depthStencilCreateInfo.back = {};

    // Color Blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {};
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendCreateInfo.attachmentCount = 1;
    colorBlendCreateInfo.pAttachments = &colorBlendAttachment;

    // Dynamic State
    vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.scissorCount = 1;

    // Pipeline Creation
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;

    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

    pipelineCreateInfo.layout = layout;
    pipelineCreateInfo.renderPass = renderpass;
    pipelineCreateInfo.subpass = 0;

    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    VkResult graphics_pipeline_creation_result = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE,
                                                                           1, &pipelineCreateInfo,
                                                                           nullptr, &pipeline);
    if (graphics_pipeline_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create graphics pipeline!", graphics_pipeline_creation_result);
    }

    // Particle Shader
    shaderStages[0].module = particleVertexShader;
    shaderStages[1].module = particleFragmentShader;
    pipelineCreateInfo.pStages = shaderStages;

    // Particle Input
    VkPipelineVertexInputStateCreateInfo& particleInputCreateInfo = vertexInputCreateInfo;

    auto particleBindingDescription = Particle::getBindingDescription();
    auto particleAttributeDescriptions = Particle::getAttributeDescriptions();

    particleInputCreateInfo.vertexBindingDescriptionCount = 1;
    particleInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(particleAttributeDescriptions.size());
    particleInputCreateInfo.pVertexBindingDescriptions = &particleBindingDescription;
    particleInputCreateInfo.pVertexAttributeDescriptions = particleAttributeDescriptions.data();

    // Particle Input Assembly
    VkPipelineInputAssemblyStateCreateInfo& particleInputAssemblyCreateInfo = inputAssemblyCreateInfo;
    particleInputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

    // Particle Pipeline Creation
    pipelineCreateInfo.pVertexInputState = &particleInputCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &particleInputAssemblyCreateInfo;

    VkResult particle_pipeline_creation_result = vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE,
                                                                           1, &pipelineCreateInfo,
                                                                           nullptr, &particlepipeline);
    if (particle_pipeline_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create particle pipeline!", graphics_pipeline_creation_result);
    }

}

GraphicsPipeline::~GraphicsPipeline() {
    if (pipeline) {
        vkDestroyPipeline(_device, pipeline, nullptr);
    }
    if (particlepipeline) {
        vkDestroyPipeline(_device, particlepipeline, nullptr);
    }
    if (renderpass) {
        vkDestroyRenderPass(_device, renderpass, nullptr);
    }
    if (descriptorsetlayout) {
        vkDestroyDescriptorSetLayout(_device, descriptorsetlayout, nullptr);
    }
    if (layout) {
        vkDestroyPipelineLayout(_device, layout, nullptr);
    }
}

GraphicsPipeline::GraphicsPipeline(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat, VkSampleCountFlagBits msaaSamples,
                                   std::string vertexShaderFilename, std::string fragmentShaderFilename,
                                   std::string particleVertexShaderFilename, std::string particleFragmentShaderFilename)
: _device(device), _format(swapchainFormat), _depthformat(depthFormat), _msaasamples(msaaSamples),
_vertshadername(vertexShaderFilename), _fragshadername(fragmentShaderFilename),
_vertparticleshadername(particleVertexShaderFilename), _fragparticleshadername(particleFragmentShaderFilename),
pipeline(nullptr), renderpass(nullptr), layout(nullptr) {}

/// Compute Pipeline ///
VkVertexInputBindingDescription Particle::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Particle);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> Particle::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Particle, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Particle, color);

    return attributeDescriptions;
}

void ComputePipeline::create() {
    VkShaderModule computeShader = loadShader(_device, _computeshadername);

    initDescriptorSetLayout();
    initLayout();
    initPipeline(computeShader);

    vkDestroyShaderModule(_device, computeShader, nullptr);
}

void ComputePipeline::initDescriptorSetLayout() {
    std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings = {};
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[0].pImmutableSamplers = nullptr;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[1].pImmutableSamplers = nullptr;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layoutBindings[2].binding = 2;
    layoutBindings[2].descriptorCount = 1;
    layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[2].pImmutableSamplers = nullptr;
    layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = 3;
    layoutCreateInfo.pBindings = layoutBindings.data();

    VkResult descriptor_set_layout_creation_result = vkCreateDescriptorSetLayout(_device, &layoutCreateInfo,nullptr, &descriptorsetlayout);
    if (descriptor_set_layout_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create compute descriptor set layout!", descriptor_set_layout_creation_result);
    }
}

void ComputePipeline::initLayout() {
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorsetlayout;

    VkResult layout_creation_result = vkCreatePipelineLayout(_device, &pipelineLayoutCreateInfo, nullptr, &layout);
    if (layout_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create compute pipeline layout creation!", layout_creation_result);
    }
}

void ComputePipeline::initPipeline(VkShaderModule computeShader) {
    VkPipelineShaderStageCreateInfo computeShaderStageInfo = {};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = computeShader;
    computeShaderStageInfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo = {};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.layout = layout;
    computePipelineCreateInfo.stage = computeShaderStageInfo;

    VkResult compute_pipeline_creation_result = vkCreateComputePipelines(_device, nullptr,1, &computePipelineCreateInfo, nullptr, &pipeline);
    if (compute_pipeline_creation_result != VK_SUCCESS) {
        throw vulkan_error("Failed to create compute pipeline!", compute_pipeline_creation_result);
    }
}

ComputePipeline::~ComputePipeline() {
    if (pipeline) {
        vkDestroyPipeline(_device, pipeline, nullptr);
    }
    if (descriptorsetlayout) {
        vkDestroyDescriptorSetLayout(_device, descriptorsetlayout, nullptr);
    }
    if (layout) {
        vkDestroyPipelineLayout(_device, layout, nullptr);
    }
}

ComputePipeline::ComputePipeline(VkDevice device, std::string computeShaderFilename)
: _device(device), _computeshadername(computeShaderFilename) {}