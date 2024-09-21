cmake_minimum_required(VERSION 3.10)
project(OptionPricingApp)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Suppress deprecated warnings (optional)
add_compile_options(-Wno-deprecated-declarations)

# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Include directories for Dear ImGui
include_directories(imgui)
include_directories(imgui/backends)
include_directories(external/glad/include) # GLAD include directory

# Include directories for matplotlib-cpp
include_directories(matplotlib-cpp)

# Include directories for stb_image
include_directories(include)

# Find Python3 and NumPy
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
if(NOT Python3_FOUND)
    message(FATAL_ERROR "Python3 not found")
endif()

# Execute Python command to get NumPy include directory
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import numpy; print(numpy.get_include())"
    OUTPUT_VARIABLE NUMPY_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Include Python3 and NumPy headers
include_directories(${Python3_INCLUDE_DIRS} ${NUMPY_INCLUDE_DIR})

# Add source files
set(SOURCES
    main.cpp
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_sdl2.cpp
    imgui/backends/imgui_impl_opengl3.cpp
    external/glad/src/glad.c # GLAD source
)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Link libraries
if (APPLE)
    set(LIBS ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} Python3::Python)
elseif (WIN32)
    set(LIBS ${SDL2_LIBRARIES} opengl32 Python3::Python)
else()
    set(LIBS ${SDL2_LIBRARIES} GL Python3::Python)
endif()

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD) # Define GLAD loader

# Add executable
add_executable(OptionPricingApp ${SOURCES})

# Link libraries
target_link_libraries(OptionPricingApp ${LIBS})
