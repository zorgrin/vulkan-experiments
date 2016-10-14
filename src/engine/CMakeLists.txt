cmake_minimum_required(VERSION 3.1)

PROJECT(vulkan-engine CXX)

FIND_PACKAGE(Qt5Core 5.5 REQUIRED)
FIND_PACKAGE(Qt5Gui 5.5 REQUIRED)

IF("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    SET(PROJECT_LIBRARIES ${VULKAN_LIBRARIES} -lvulkan-wrapper)
ELSE()
    SET(PROJECT_LIBRARIES ${VULKAN_LIBRARIES} vulkan-wrapper)
ENDIF()

SET(HEADER_FILES
    Engine.h
    RenderObject.h
    ISurfaceBackend.h
    SurfaceBackend.h
    IRenderable.h
    ImageObject.h
    SimpleBufferObject.h
    RenderTemplate.h
    RenderBatch.h
    RenderObjectGroup.h
    
    Utils.h
    hardcode.h
    
    ICamera.h
    Camera.cpp
    CameraController.h
)   


SET(SOURCE_FILES
    Engine.cpp
    RenderObject.cpp
    SurfaceBackend.cpp
    ImageObject.cpp
    SimpleBufferObject.cpp
    RenderTemplate.cpp
    RenderBatch.cpp
    RenderObjectGroup.cpp
    
    Utils.cpp
    
    Camera.cpp
    CameraController.cpp
)

ADD_LIBRARY(${PROJECT_NAME} 
    STATIC
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

LINK_DIRECTORIES(${LIBRARY_OUTPUT_PATH}/${OUTPUT_DIR_SUFFIX})

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
    ${QT_LIBRARIES}
    ${Qt5Core_LIBRARIES}
    ${Qt5Gui_LIBRARIES}
    ${PROJECT_LIBRARIES}
)
