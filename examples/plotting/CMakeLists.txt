cmake_minimum_required(VERSION 3.10)

# set the project name
project(cplot)

# Add graphviz as a submodule

set(GRAPHVIZ_DIR "/usr" CACHE PATH "Path to Graphviz installation")

find_path(GRAPHVIZ_INCLUDE_DIR graphviz/gvc.h HINTS ${GRAPHVIZ_DIR} ENV GRAPHVIZ_DIR PATH_SUFFIXES include)

find_library(GRAPHVIZ_cdt_LIBRARY NAMES cdt HINTS ${GRAPHVIZ_DIR} ENV GRAPHVIZ_DIR PATH_SUFFIXES lib)
find_library(GRAPHVIZ_gvc_LIBRARY NAMES gvc HINTS ${GRAPHVIZ_DIR} ENV GRAPHVIZ_DIR PATH_SUFFIXES lib)
find_library(GRAPHVIZ_graph_LIBRARY NAMES graph cgraph HINTS ${GRAPHVIZ_DIR} ENV GRAPHVIZ_DIR PATH_SUFFIXES lib)

if (NOT GRAPHVIZ_INCLUDE_DIR OR NOT GRAPHVIZ_cdt_LIBRARY OR NOT GRAPHVIZ_gvc_LIBRARY OR NOT GRAPHVIZ_graph_LIBRARY)
    message(FATAL_ERROR "Graphviz not found. Please set the GRAPHVIZ_DIR variable to the Graphviz installation directory.")
endif()

set(GRAPHVIZ_LIBRARIES ${GRAPHVIZ_gvc_LIBRARY} ${GRAPHVIZ_graph_LIBRARY} ${GRAPHVIZ_cdt_LIBRARY} ${GRAPHVIZ_pathplan_LIBRARY})

add_executable(cpp_plot cplot.cpp)
target_link_libraries(cpp_plot PUBLIC ${GRAPHVIZ_LIBRARIES})
set_target_properties(cpp_plot PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)