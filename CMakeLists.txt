cmake_minimum_required(VERSION 3.0)

project(QwtWaterfallplotExample)

find_package(Qt5Widgets         REQUIRED)
find_package(Qt5Gui             REQUIRED)
find_package(Qt5Core            REQUIRED)
find_package(Qt5SerialPort      REQUIRED)
find_package(Qt5PrintSupport    REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=gnu++0x")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wpedantic -g -O0 -std=gnu++0x")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -Wall -Wextra -Wpedantic -std=gnu++0x")

# QWT
INCLUDE(FindQwt.cmake)
find_library(Qwt REQUIRED)
include_directories(${QWT_INCLUDE_DIR})

include_directories(${QT_INCLUDES})
include_directories(${QWT_HEADERS})
include_directories(.)

# ==============================================================================
# Source
# ==============================================================================
set(APP_SOURCE main.cpp Waterfallplot.cpp ExportDialog.cpp)
set(UISrcs ExportDialog.ui)

# ==============================================================================
# Target
# ==============================================================================
add_executable(qwtwaterfallplot ${APP_SOURCE} ${UISrcs} ${MOCSrcs})

set_target_properties(qwtwaterfallplot PROPERTIES
                    AUTOMOC TRUE
                    AUTORCC TRUE
                    AUTOUIC TRUE)

target_link_libraries(qwtwaterfallplot Qt5::Core Qt5::Gui Qt5::Widgets Qt5::SerialPort Qt5::PrintSupport
                      ${QWT_LIBRARY})

target_include_directories(qwtwaterfallplot PRIVATE
                          ${CMAKE_CURRENT_BINARY_DIR}
                          ${CMAKE_CURRENT_SOURCE_DIR}
                          ${CMAKE_SOURCE_DIR})


set_property(TARGET qwtwaterfallplot PROPERTY C_STANDARD 99)
