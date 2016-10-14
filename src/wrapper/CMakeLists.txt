cmake_minimum_required(VERSION 3.1)

PROJECT(vulkan-wrapper CXX)

FIND_PACKAGE(Qt5Core 5.5 REQUIRED)
FIND_PACKAGE(Qt5Gui 5.5 REQUIRED)

IF("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    FIND_PACKAGE(Qt5X11Extras 5.5 REQUIRED)
    SET(PROJECT_LIBRARIES ${Qt5X11Extras_LIBRARIES})
ELSE()
    SET(PROJECT_LIBRARIES )    
ENDIF()

SET(HEADER_FILES
    defines.h
    struct-validator.hpp
    exceptions.hpp
    Handle.hpp
    reference-list.hpp
    Instance.h
    PhysicalDevice.h
    Surface.h
    Queue.h
    Device.h
    Swapchain.h
    CommandPool.h
    CommandBuffer.h
    Allocator.h
    DeviceMemory.h
    Buffer.h
    Image.h
    ImageView.h
    Fence.h
    Semaphore.h
    RenderPass.h
    Framebuffer.h
    ShaderModule.h
    Pipeline.h
    PipelineLayout.h
    PipelineCache.h
    DescriptorSet.h
    DescriptorPool.h
    DescriptorSetLayout.h    
    Sampler.h

    DebugReport.h
)

SET(SOURCE_FILES
    Instance.cpp
    PhysicalDevice.cpp
    Surface.cpp
    Queue.cpp
    Device.cpp
    Swapchain.cpp
    CommandPool.cpp
    CommandBuffer.cpp
    Allocator.cpp
    DeviceMemory.cpp
    Buffer.cpp
    Image.cpp
    ImageView.cpp
    Fence.cpp
    Semaphore.cpp
    RenderPass.cpp
    Framebuffer.cpp
    ShaderModule.cpp
    Pipeline.cpp
    PipelineLayout.cpp
    PipelineCache.cpp
    DescriptorPool.cpp
    DescriptorSet.cpp
    DescriptorSetLayout.cpp    
    Sampler.cpp
    
    DebugReport.cpp
)

INCLUDE_DIRECTORIES(${Qt5Core_INCLUDE_DIRS})

ADD_LIBRARY(${PROJECT_NAME} 
    STATIC
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
    ${QT_LIBRARIES}
    ${Qt5Core_LIBRARIES}
    ${Qt5Gui_LIBRARIES}
    ${PROJECT_LIBRARIES}
    ${VULKAN_LIBRARIES}
)

