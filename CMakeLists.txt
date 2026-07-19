cmake_minimum_required (VERSION 3.29)

set (TEMPLATE_NAME "VulkanEngine")

project (${TEMPLATE_NAME})

# Add option to enable/disable C++ 20 module
option(ENABLE_CPP20_MODULE "Enable C++ 20 module support for Vulkan" OFF)

# Enable C++ module dependency scanning only if C++ 20 module is enabled
if(ENABLE_CPP20_MODULE)
  set(CMAKE_CXX_SCAN_FOR_MODULES ON)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
find_package(tinyobjloader REQUIRED)
find_package(tinygltf REQUIRED)
find_package(KTX REQUIRED) # <-- El paquete se encuentra correctamente aquí

# Set up Vulkan C++ module only if enabled
if(ENABLE_CPP20_MODULE)
  add_library(VulkanCppModule)
  add_library(Vulkan::cppm ALIAS VulkanCppModule)

  target_compile_definitions(VulkanCppModule PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1)
  target_include_directories(VulkanCppModule PUBLIC "${Vulkan_INCLUDE_DIR}")
  target_link_libraries(VulkanCppModule PUBLIC Vulkan::Vulkan)

  set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)

  if(MSVC)
    target_compile_options(VulkanCppModule PRIVATE
            /std:c++latest
            /permissive-
            /Zc:__cplusplus
            /EHsc
            /Zc:preprocessor
            /translateInclude
    )
  endif()

  target_sources(VulkanCppModule
          PUBLIC
          FILE_SET cxx_modules TYPE CXX_MODULES
          BASE_DIRS
          "${Vulkan_INCLUDE_DIR}"
          FILES
          "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
  )

  target_sources(VulkanCppModule
          PRIVATE
          "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
  )
else()
  # CORREGIDO: Una librería INTERFACE no debe listar archivos fuente aquí
  add_library(VulkanCppModule INTERFACE
          src/GameObject.cpp
          src/GameObject.hpp
          src/resources/Texture.hpp
          src/services/GraphicsServices.hpp
          src/VulkanUtils.hpp
          src/rendergraph/RenderGraph.cpp
          src/rendergraph/RenderGraph.hpp)
  add_library(Vulkan::cppm ALIAS VulkanCppModule)
  target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
  target_compile_definitions(VulkanCppModule
          INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
  )
endif()

find_package(stb REQUIRED)
set(STB_INCLUDEDIR ${stb_INCLUDE_DIRS})

find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)

set(TEMPLATE_DIR ${CMAKE_BINARY_DIR}/${TEMPLATE_NAME})

# CORREGIDO: Eliminado main.cpp duplicado y añadido StbImplementation.cpp
add_executable (${TEMPLATE_NAME}
        src/main.cpp
        src/Vertex.hpp
        src/Engine.cpp
        src/Engine.hpp
        src/MacrosImplementation.cpp
        src/components/Component.hpp
        src/Entity.hpp
        src/components/TransformComponent.hpp
        src/resources/ResourceHandle.hpp
        src/resources/Resource.hpp
        src/resources/ResourceManager.hpp
        src/resources/Texture.hpp
        src/resources/Texture.cpp
        src/VulkanUtils.cpp
)

set_target_properties (${TEMPLATE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${TEMPLATE_DIR})
set_target_properties (${TEMPLATE_NAME} PROPERTIES CXX_STANDARD 20)

# CORREGIDO: Añadido el target de KTX (KTX::ktx) para que el linker lo encuentre
target_link_libraries (${TEMPLATE_NAME}
        Vulkan::cppm
        glfw
        tinyobjloader::tinyobjloader
        KTX::ktx
)

# CORREGIDO: Añadido el directorio de includes de tinyobjloader y KTX si el sistema lo requiere
target_include_directories (${TEMPLATE_NAME} PRIVATE
        ${STB_INCLUDEDIR}
        ${tinyobjloader_INCLUDE_DIRS}
        ${KTX_INCLUDE_DIRS}
)

# Add compile definition if C++ 20 module is enabled
if(ENABLE_CPP20_MODULE)
  target_compile_definitions(${TEMPLATE_NAME} PRIVATE USE_CPP20_MODULES=1)
endif()

# Define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS to treat VK_ERROR_OUT_OF_DATE_KHR as a success code
target_compile_definitions(${TEMPLATE_NAME} PRIVATE "VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS" )

if(WIN32)
  if(${CMAKE_GENERATOR} MATCHES "Visual Studio.*")
    set_target_properties(${TEMPLATE_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${TEMPLATE_DIR})
  endif()
endif()

# Slang shader
set (SHADER_TARGET ${TEMPLATE_NAME}Shader)
set (SHADERS_DIR ${TEMPLATE_DIR}/shaders)
set (SHADER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/shader.slang")
set (ENTRY_POINTS -entry vertMain -entry fragMain)
add_custom_command(OUTPUT ${SHADERS_DIR} COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR})
add_custom_command(
        OUTPUT  ${SHADERS_DIR}/slang.spv
        COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o ${TEMPLATE_DIR}/shaders/slang.spv
        WORKING_DIRECTORY ${SHADERS_DIR}
        DEPENDS ${SHADERS_DIR} ${SHADER_SOURCES}
        COMMENT "Compiling Slang Shaders"
        VERBATIM
)
add_custom_target(${SHADER_TARGET} DEPENDS ${SHADERS_DIR}/slang.spv)
add_dependencies(${TEMPLATE_NAME} ${SHADER_TARGET})

# Assets
set (ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/")
file(COPY "${ASSET_DIR}/" DESTINATION ${TEMPLATE_DIR}/assets)