cmake_minimum_required(VERSION 3.1)
project(DCode)

add_definitions(-DANDROID_PLATFORM)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(DCode ${SOURCE_FILES}
               physical_Android.cpp
               utils/semaphore.h
               screen_fetcher_Android.h
               camera_fetcher_Android.h
               utils/blocking_queue.h
               screen_fetcher_Android.cpp
               utils/blocking_set.h
               camera_fetcher_Android.cpp
               locator_scanner.cpp
               pattern_matcher.h
               utils/RGB.h
               utils/constants.h
               palette.h
               utils/utils.h
               test/utils_test.h
               test/utils_test.cpp
               config.h
               datalink.h
               test/physical_test.h
               test/physical_test.cpp
               screen_painter_Android.h
               screen_painter_Android.cpp
               test/main.cpp
               symbol_scanner.h
               symbol_scanner.cpp modulator.h
               modulator.cpp
               utils/escaper.h
               structures.h
               adaptive_parameters.h
               packet.h
               packet.cpp
               pixel_reader_Android.h
               utils/reed_solomon_code.cpp
               utils/reed_solomon_code.h
               utils/hamming128.h
               frame_painter.h rx_buffer.h segment_pool.h tx_buffer.h rx_buffer.cpp tx_buffer.cpp ack_buffer.h ack_buffer.cpp adaptation.h adaptation.cpp)