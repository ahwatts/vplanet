// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include <array>
#include <cstring>
#include <sstream>
#include <vector>

#include "../vulkan.h"
#include "../VmaUsage.h"

#include "Pipeline.h"
#include "Renderer.h"
#include "Resource.h"
#include "System.h"
#include "Uniforms.h"

const std::vector<unsigned char> &TERRAIN_SLANG_SHADER_BYTECODE = LOAD_RESOURCE(terrain_slang_spv);
const std::vector<unsigned char> &TERRAIN_GLSL_VERTEX_SHADER_BYTECODE = LOAD_RESOURCE(terrain_vert_spv);
const std::vector<unsigned char> &TERRAIN_GLSL_FRAGMENT_SHADER_BYTECODE = LOAD_RESOURCE(terrain_frag_spv);

gfx::TerrainPipeline::TerrainPipeline()
: Pipeline{},
  m_uniform_set{},
  m_num_indices{0},
  m_vertex_buffer{nullptr},
  m_index_buffer{nullptr},
  m_vertex_buffer_allocation{nullptr},
  m_index_buffer_allocation{nullptr}
{}  

gfx::TerrainPipeline::TerrainPipeline(Renderer *renderer) : TerrainPipeline() {
    m_renderer = renderer;
    m_uniform_set = ModelUniformSet(&m_renderer->system()->uniforms());
    initPipeline();
}

gfx::TerrainPipeline::~TerrainPipeline() {
    if (m_renderer != nullptr) {
        if (m_vertex_buffer_allocation != nullptr) {
            vmaFreeMemory(m_renderer->system()->allocator(), m_vertex_buffer_allocation);
            m_vertex_buffer_allocation = nullptr;
        }

        if (m_index_buffer_allocation != nullptr) {
            vmaFreeMemory(m_renderer->system()->allocator(), m_index_buffer_allocation);
            m_index_buffer_allocation = nullptr;
        }
    }
}

void gfx::TerrainPipeline::setGeometry(const std::vector<TerrainVertex> &verts, const std::vector<uint32_t> &indices) {
    System *gfx = m_renderer->system();

    if (m_vertex_buffer_allocation != nullptr) {
        vmaFreeMemory(m_renderer->system()->allocator(), m_vertex_buffer_allocation);
        m_vertex_buffer_allocation = nullptr;
    }

    if (m_index_buffer_allocation != nullptr) {
        vmaFreeMemory(m_renderer->system()->allocator(), m_index_buffer_allocation);
        m_index_buffer_allocation = nullptr;
    }

    std::tie(m_vertex_buffer, m_vertex_buffer_allocation) = gfx->createBufferWithData(
        verts.data(), verts.size() * sizeof(TerrainVertex),
        vk::BufferUsageFlagBits::eVertexBuffer, 0,
        "terrain vertex"
    );

    std::tie(m_index_buffer, m_index_buffer_allocation) = gfx->createBufferWithData(
        indices.data(), indices.size() * sizeof(uint32_t),
        vk::BufferUsageFlagBits::eIndexBuffer, 0,
        "terrain index"
    );

    m_num_indices = static_cast<uint32_t>(indices.size());
}

void gfx::TerrainPipeline::setTransform(const glm::mat4x4 &xform) {
    m_uniform_set.setTransform(xform);
}

void gfx::TerrainPipeline::writeTransform(uint32_t buffer_index) {
    m_uniform_set.updateModelBuffer(buffer_index);
}

void gfx::TerrainPipeline::recordCommands(const vk::raii::CommandBuffer &cmd_buf, uint32_t frame_index) {
    const vk::raii::PipelineLayout &layout = m_renderer->pipelineLayout();
    const std::vector<vk::raii::DescriptorSet> &model_uniforms = m_uniform_set.descriptorSets();

    cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
    cmd_buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *layout, 1, *model_uniforms[frame_index], nullptr);
    cmd_buf.bindVertexBuffers(0, *m_vertex_buffer, {0});
    cmd_buf.bindIndexBuffer(*m_index_buffer, 0, vk::IndexType::eUint32);
    cmd_buf.drawIndexed(m_num_indices, 1, 0, 0, 0);
}

void gfx::TerrainPipeline::initPipeline() {
    System *system = m_renderer->system();
    const vk::raii::Device &device = system->device();
    vk::Extent2D extent = system->swapchain().extent();
    const vk::raii::PipelineLayout &layout = m_renderer->pipelineLayout();
    vk::SurfaceFormatKHR swapchain_format = system->swapchain().format();
    vk::Format depth_format = system->depthBuffer().format();

    vk::ShaderModuleCreateInfo sm_ci{
        .codeSize = TERRAIN_SLANG_SHADER_BYTECODE.size() * sizeof(std::remove_reference<decltype(TERRAIN_SLANG_SHADER_BYTECODE)>::type::value_type),
        .pCode = reinterpret_cast<const uint32_t *>(TERRAIN_SLANG_SHADER_BYTECODE.data()),
    };
    vk::raii::ShaderModule shader = device.createShaderModule(sm_ci);

    std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages{
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = *shader,
            .pName = "vs_main",
        },
        vk::PipelineShaderStageCreateInfo{
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *shader,
            .pName = "fs_main",
        },
    };

    // vk::ShaderModuleCreateInfo vsm_ci{
    //     .codeSize = TERRAIN_GLSL_VERTEX_SHADER_BYTECODE.size() * sizeof(std::remove_reference<decltype(TERRAIN_GLSL_VERTEX_SHADER_BYTECODE)>::type::value_type),
    //     .pCode = reinterpret_cast<const uint32_t *>(TERRAIN_GLSL_VERTEX_SHADER_BYTECODE.data()),
    // };
    // vk::raii::ShaderModule vertex_shader = device.createShaderModule(vsm_ci);

    // vk::ShaderModuleCreateInfo fsm_ci{
    //     .codeSize = TERRAIN_GLSL_FRAGMENT_SHADER_BYTECODE.size() * sizeof(std::remove_reference<decltype(TERRAIN_GLSL_FRAGMENT_SHADER_BYTECODE)>::type::value_type),
    //     .pCode = reinterpret_cast<const uint32_t *>(TERRAIN_GLSL_FRAGMENT_SHADER_BYTECODE.data()),
    // };
    // vk::raii::ShaderModule fragment_shader = device.createShaderModule(fsm_ci);

    // std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages{
    //     vk::PipelineShaderStageCreateInfo{
    //         .stage = vk::ShaderStageFlagBits::eVertex,
    //         .module = *vertex_shader,
    //         .pName = "main",
    //     },
    //     vk::PipelineShaderStageCreateInfo{
    //         .stage = vk::ShaderStageFlagBits::eFragment,
    //         .module = *fragment_shader,
    //         .pName = "main",
    //     },
    // };

    vk::VertexInputBindingDescription bind_desc = TerrainVertex::bindingDescription();
    std::array<vk::VertexInputAttributeDescription, TerrainVertex::NUM_ATTRIBUTES> attr_desc = TerrainVertex::attributeDescription();
    vk::PipelineVertexInputStateCreateInfo vertex_input_ci = vk::PipelineVertexInputStateCreateInfo{}
        .setVertexBindingDescriptions(bind_desc)
        .setVertexAttributeDescriptions(attr_desc);
    
    vk::PipelineInputAssemblyStateCreateInfo input_assembly_ci{
        .topology = vk::PrimitiveTopology::eTriangleList,
    };
    
    vk::PipelineViewportStateCreateInfo viewport_ci{
        .viewportCount = 1,
        .scissorCount = 1,
    };

    vk::PipelineRasterizationStateCreateInfo raster_ci{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .lineWidth = 1.0f,
    };

    vk::PipelineMultisampleStateCreateInfo multisample_state_ci{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False,
    };

    vk::PipelineDepthStencilStateCreateInfo depth_ci{
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable = vk::False,
    };

    vk::PipelineColorBlendAttachmentState blend_attachment{
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | 
            vk::ColorComponentFlagBits::eG | 
            vk::ColorComponentFlagBits::eB | 
            vk::ColorComponentFlagBits::eA,
    };

    vk::PipelineColorBlendStateCreateInfo color_blend_ci = vk::PipelineColorBlendStateCreateInfo{
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
    }.setAttachments(blend_attachment);

    std::array<vk::DynamicState, 2> dynamic_states{
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    vk::PipelineDynamicStateCreateInfo dynamic_state_ci = vk::PipelineDynamicStateCreateInfo{}
        .setDynamicStates(dynamic_states);

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipeline_ci{
        vk::GraphicsPipelineCreateInfo{
            .pVertexInputState = &vertex_input_ci,
            .pInputAssemblyState = &input_assembly_ci,
            .pViewportState = &viewport_ci,
            .pRasterizationState = &raster_ci,
            .pMultisampleState = &multisample_state_ci,
            .pDepthStencilState = &depth_ci,
            .pColorBlendState = &color_blend_ci,
            .pDynamicState = &dynamic_state_ci,
            .layout = *layout,
            .renderPass = nullptr,
        },
        vk::PipelineRenderingCreateInfo{
            .depthAttachmentFormat = depth_format,
        },
    };
    pipeline_ci.get<vk::GraphicsPipelineCreateInfo>().setStages(shader_stages);
    pipeline_ci.get<vk::PipelineRenderingCreateInfo>()
        .setColorAttachmentFormats(swapchain_format.format);
    
    m_pipeline = device.createGraphicsPipeline(nullptr, pipeline_ci.get<vk::GraphicsPipelineCreateInfo>());
}
