#pragma once

namespace engine
{
namespace render
{
namespace vk
{
    inline void validateStruct(const VkOffset2D& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkOffset3D& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkExtent2D& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkExtent3D& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkViewport& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkRect2D& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkClearRect& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkComponentMapping& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkPhysicalDeviceProperties& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkExtensionProperties& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkLayerProperties& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkApplicationInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_APPLICATION_INFO);
    }

    inline void validateStruct(const VkAllocationCallbacks& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkDeviceQueueCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
    }

    inline void validateStruct(const VkDeviceCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
    }

    inline void validateStruct(const VkInstanceCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
    }

    inline void validateStruct(const VkQueueFamilyProperties& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkPhysicalDeviceMemoryProperties& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkMemoryAllocateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
    }

    inline void validateStruct(const VkMemoryRequirements& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkSparseImageFormatProperties& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkSparseImageMemoryRequirements& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkMemoryType& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkMemoryHeap& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkMappedMemoryRange& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE);
        assert(type.memory != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkFormatProperties& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkImageFormatProperties& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkDescriptorBufferInfo& type)
    {
        assert(type.buffer != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkDescriptorImageInfo& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkWriteDescriptorSet& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
        assert(type.dstSet != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkCopyDescriptorSet& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET);
        assert(type.srcSet != VK_NULL_HANDLE);
        assert(type.dstSet != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkBufferCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
    }

    inline void validateStruct(const VkBufferViewCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO);
        assert(type.buffer != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkImageSubresource& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkImageSubresourceLayers& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkImageSubresourceRange& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkMemoryBarrier& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_MEMORY_BARRIER);
    }

    inline void validateStruct(const VkBufferMemoryBarrier& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER);
        assert(type.buffer != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkImageMemoryBarrier& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
        assert(type.image != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkImageCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
    }

    inline void validateStruct(const VkSubresourceLayout& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkImageViewCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
        assert(type.image != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkBufferCopy& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkSparseMemoryBind& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkSparseImageMemoryBind& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkSparseBufferMemoryBindInfo& type)
    {
        assert(type.buffer != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkSparseImageOpaqueMemoryBindInfo& type)
    {
        assert(type.image != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkSparseImageMemoryBindInfo& type)
    {
        assert(type.image != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkBindSparseInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_BIND_SPARSE_INFO);
    }

    inline void validateStruct(const VkImageCopy& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkImageBlit& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkBufferImageCopy& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkImageResolve& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkShaderModuleCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
    }

    inline void validateStruct(const VkDescriptorSetLayoutBinding& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkDescriptorSetLayoutCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
    }

    inline void validateStruct(const VkDescriptorPoolSize& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkDescriptorPoolCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
    }

    inline void validateStruct(const VkDescriptorSetAllocateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
        assert(type.descriptorPool != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkSpecializationMapEntry& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkSpecializationInfo& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkPipelineShaderStageCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
        assert(type.module != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkComputePipelineCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO);
        assert(type.layout != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkVertexInputBindingDescription& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkVertexInputAttributeDescription& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkPipelineVertexInputStateCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
    }

    inline void validateStruct(const VkPipelineInputAssemblyStateCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
    }

    inline void validateStruct(const VkPipelineTessellationStateCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO);
    }

    inline void validateStruct(const VkPipelineViewportStateCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
    }

    inline void validateStruct(const VkPipelineRasterizationStateCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
    }

    inline void validateStruct(const VkPipelineMultisampleStateCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
    }

    inline void validateStruct(const VkPipelineColorBlendAttachmentState& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkPipelineColorBlendStateCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
    }

    inline void validateStruct(const VkPipelineDynamicStateCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
    }

    inline void validateStruct(const VkStencilOpState& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkPipelineDepthStencilStateCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
    }

    inline void validateStruct(const VkGraphicsPipelineCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
        assert(type.layout != VK_NULL_HANDLE);
        assert(type.renderPass != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkPipelineCacheCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO);
    }

    inline void validateStruct(const VkPushConstantRange& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkPipelineLayoutCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
    }

    inline void validateStruct(const VkSamplerCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
    }

    inline void validateStruct(const VkCommandPoolCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
    }

    inline void validateStruct(const VkCommandBufferAllocateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
        assert(type.commandPool != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkCommandBufferInheritanceInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO);
    }

    inline void validateStruct(const VkCommandBufferBeginInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
    }

    inline void validateStruct(const VkRenderPassBeginInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
        assert(type.renderPass != VK_NULL_HANDLE);
        assert(type.framebuffer != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkClearDepthStencilValue& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkClearAttachment& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkAttachmentDescription& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkAttachmentReference& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkSubpassDescription& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkSubpassDependency& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkRenderPassCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
    }

    inline void validateStruct(const VkEventCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_EVENT_CREATE_INFO);
    }

    inline void validateStruct(const VkFenceCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
    }

    inline void validateStruct(const VkPhysicalDeviceFeatures& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkPhysicalDeviceSparseProperties& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkPhysicalDeviceLimits& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkSemaphoreCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
    }

    inline void validateStruct(const VkQueryPoolCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO);
    }

    inline void validateStruct(const VkFramebufferCreateInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
        assert(type.renderPass != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkDrawIndirectCommand& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkDrawIndexedIndirectCommand& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkDispatchIndirectCommand& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkSubmitInfo& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_SUBMIT_INFO);
    }

    inline void validateStruct(const VkDisplayPropertiesKHR& type)
    {
        assert(type.display != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkDisplayPlanePropertiesKHR& type)
    {
        assert(type.currentDisplay != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkDisplayModeParametersKHR& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkDisplayModePropertiesKHR& type)
    {
        assert(type.displayMode != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkDisplayModeCreateInfoKHR& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR);
    }

    inline void validateStruct(const VkDisplayPlaneCapabilitiesKHR& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkDisplaySurfaceCreateInfoKHR& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR);
        assert(type.displayMode != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkDisplayPresentInfoKHR& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_DISPLAY_PRESENT_INFO_KHR);
    }

    inline void validateStruct(const VkSurfaceCapabilitiesKHR& type)
    {
        Q_UNUSED(type);
    }

#if defined(Q_OS_LINUX)
    inline void validateStruct(const VkXcbSurfaceCreateInfoKHR& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR);
    }
#endif

    inline void validateStruct(const VkSurfaceFormatKHR& type)
    {
        Q_UNUSED(type);
    }

    inline void validateStruct(const VkSwapchainCreateInfoKHR& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
        assert(type.surface != VK_NULL_HANDLE);
    }

    inline void validateStruct(const VkPresentInfoKHR& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
    }

    inline void validateStruct(const VkDebugReportCallbackCreateInfoEXT& type)
    {
        assert(type.sType == VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT);
    }

} //namespace vk
} //namespace render
} //namespace engine
