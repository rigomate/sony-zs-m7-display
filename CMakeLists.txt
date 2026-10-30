cmake_minimum_required(VERSION 3.16)
project(zs_m7_display_protocol LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(display_protocol
    src/saleae_csv_source.cpp
)
target_include_directories(display_protocol PUBLIC include)

add_executable(display_replay
    src/replay_main.cpp
)
target_link_libraries(display_replay PRIVATE display_protocol)

include(CTest)
if(BUILD_TESTING)
    add_executable(rec_display_test tests/rec_display_test.cpp)
    target_link_libraries(rec_display_test PRIVATE display_protocol)
    add_test(NAME captured_rec_display COMMAND rec_display_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/manual/rec-on/raw/digital.csv)
    add_executable(track21_test tests/track21_test.cpp)
    target_link_libraries(track21_test PRIVATE display_protocol)
    add_test(NAME captured_track21_display COMMAND track21_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/track21)
    add_executable(program_message_test tests/program_message_test.cpp)
    target_link_libraries(program_message_test PRIVATE display_protocol)
    add_test(NAME captured_program_messages COMMAND program_message_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/manual)
    add_executable(program_track_test tests/program_track_test.cpp)
    target_link_libraries(program_track_test PRIVATE display_protocol)
    add_test(NAME captured_program_track COMMAND program_track_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/manual/program-track-01-to-20-to-01-raw/digital.csv)
    add_executable(pgm_display_test tests/pgm_display_test.cpp)
    target_link_libraries(pgm_display_test PRIVATE display_protocol)
    add_test(NAME captured_pgm_display COMMAND pgm_display_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/manual/pgm-blinking/raw/digital.csv)
    add_executable(repeat_display_test tests/repeat_display_test.cpp)
    target_link_libraries(repeat_display_test PRIVATE display_protocol)
    add_test(NAME captured_repeat_display COMMAND repeat_display_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/manual)
    add_executable(remote_indicator_test tests/remote_indicator_test.cpp)
    target_link_libraries(remote_indicator_test PRIVATE display_protocol)
    add_test(NAME captured_remote_indicators COMMAND remote_indicator_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/remote-indicators)
    add_executable(date_display_test tests/date_display_test.cpp)
    target_link_libraries(date_display_test PRIVATE display_protocol)
    add_test(NAME captured_date_display COMMAND date_display_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/manual)
    add_executable(timer_display_test tests/timer_display_test.cpp)
    target_link_libraries(timer_display_test PRIVATE display_protocol)
    add_test(NAME captured_timer_display COMMAND timer_display_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/manual/timer-play-blinking-raw/digital.csv)
    add_executable(sleep_display_test tests/sleep_display_test.cpp)
    target_link_libraries(sleep_display_test PRIVATE display_protocol)
    add_test(NAME captured_sleep_display COMMAND sleep_display_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/manual)
    add_executable(source_display_test tests/source_display_test.cpp)
    target_link_libraries(source_display_test PRIVATE display_protocol)
    add_test(NAME captured_source_display COMMAND source_display_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/manual/cycle-md-cd-radio-linein-raw/digital.csv)
    add_executable(volume_display_test tests/volume_display_test.cpp)
    target_link_libraries(volume_display_test PRIVATE display_protocol)
    add_test(NAME captured_volume_display COMMAND volume_display_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/manual/volume-from-min-to-max-raw/digital.csv)
    add_executable(md_animation_test tests/md_animation_test.cpp)
    target_link_libraries(md_animation_test PRIVATE display_protocol)
    add_test(NAME captured_md_animation COMMAND md_animation_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/md-animation)
    add_executable(radio_display_test tests/radio_display_test.cpp)
    target_link_libraries(radio_display_test PRIVATE display_protocol)
    add_test(NAME captured_radio_display COMMAND radio_display_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/radio)
    add_executable(fixed_bit_frame_test tests/fixed_bit_frame_test.cpp)
    target_include_directories(fixed_bit_frame_test PRIVATE include)
    add_test(NAME gpio_title_capture_replay COMMAND fixed_bit_frame_test
        ${CMAKE_CURRENT_SOURCE_DIR}/captures/title-test-01-0123456789AB-raw/digital.csv
        ${CMAKE_CURRENT_SOURCE_DIR}/captures/test20/20-next-raw/digital.csv)
    add_executable(shared_select_test tests/shared_select_test.cpp)
    target_link_libraries(shared_select_test PRIVATE display_protocol)
    add_test(NAME shared_select_capture_equivalence COMMAND shared_select_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/test20)
    add_executable(live_display_test tests/live_display_test.cpp)
    target_link_libraries(live_display_test PRIVATE display_protocol)
    add_test(NAME live_display_snapshot COMMAND live_display_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/test20/20-next-raw/digital.csv)
    add_executable(display_state_test tests/display_state_test.cpp)
    target_link_libraries(display_state_test PRIVATE display_protocol)
    add_test(NAME captured_numeric_display COMMAND display_state_test ${CMAKE_CURRENT_SOURCE_DIR}/captures/test20)
    add_executable(title_test tests/title_test.cpp)
    target_link_libraries(title_test PRIVATE display_protocol)
    add_test(NAME captured_title_decode COMMAND title_test ${CMAKE_CURRENT_SOURCE_DIR}/captures)
    add_executable(replay_test
        tests/replay_test.cpp
    )
    target_link_libraries(replay_test PRIVATE display_protocol)
    add_test(
        NAME captured_track_replay
        COMMAND replay_test
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/per-track/track-01-after-play-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/per-track/track-02-next-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/per-track/track-03-next-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/per-track/track-04-next-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/per-track/track-05-next-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/per-track/track-06-next-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/per-track/track-07-next-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/per-track/track-08-next-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/per-track/track-09-next-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/per-track/track-10-next-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/per-track/track-11-next-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/per-track/track-12-next-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/title-test-01-0123456789AB-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/title-test-02-CDEFGHIJKLMN-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/title-test-03-OPQRSTUVWXYZ-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/title-test-04-abcdefghijkl-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/title-test-05-mnopqrstuvwx-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/title-test-06-yz-punct-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/title-test-07-reference-b-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/title-test-08-reference-H-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/title-test-09-PLAY-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/title-test-10-TRACK-10-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/title-test-11-0123456789-raw/digital.csv
            ${CMAKE_CURRENT_SOURCE_DIR}/captures/title-test-12-long-title-raw/digital.csv
    )
endif()