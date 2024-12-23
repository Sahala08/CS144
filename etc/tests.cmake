include(CTest)

list(APPEND CMAKE_CTEST_ARGUMENTS --output-on-failure --stop-on-failure --timeout 12 -E 'speed_test|optimization|webget')

set(compile_name "compile with bug-checkers")
add_test(NAME ${compile_name}
  COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" -t functionality_testing webget)

macro (ttest name)
  add_test(NAME ${name} COMMAND "${name}_sanitized")
  set_property(TEST ${name} PROPERTY FIXTURES_REQUIRED compile)
endmacro (ttest)

# macro(add_debug_test name)
#   add_custom_target(debug_${name}
#     COMMAND gdb --args $(CMAKE_CTEST_COMMAND) --output-on-failure --timeout 12 -R "^${name}"
#     COMMENT "Starting GDB for debugging ${name}"
#   )
# endmacro( add_debug_test name)


set_property(TEST ${compile_name} PROPERTY TIMEOUT 0)
set_tests_properties(${compile_name} PROPERTIES FIXTURES_SETUP compile)

add_test(NAME t_webget COMMAND "${PROJECT_SOURCE_DIR}/tests/webget_t.sh" "${PROJECT_BINARY_DIR}")
set_property(TEST t_webget PROPERTY FIXTURES_REQUIRED compile)

ttest(byte_stream_basics)
ttest(byte_stream_capacity)
ttest(byte_stream_one_write)
ttest(byte_stream_two_writes)
ttest(byte_stream_many_writes)
ttest(byte_stream_stress_test)

ttest(reassembler_single)
ttest(reassembler_cap)
ttest(reassembler_seq)
ttest(reassembler_dup)
ttest(reassembler_holes)
ttest(reassembler_overlapping)
ttest(reassembler_win)

ttest(wrapping_integers_cmp)
ttest(wrapping_integers_wrap)
ttest(wrapping_integers_unwrap)
ttest(wrapping_integers_roundtrip)
ttest(wrapping_integers_extra)

ttest(recv_connect)
ttest(recv_transmit)
ttest(recv_window)
ttest(recv_reorder)
ttest(recv_reorder_more)
ttest(recv_close)
ttest(recv_special)

ttest(send_connect)
ttest(send_transmit)
ttest(send_retx)
ttest(send_window)
ttest(send_ack)
ttest(send_close)
ttest(send_extra)

ttest(net_interface)

ttest(router)

# # 为每个具体的测试添加调试目标
# add_debug_test(byte_stream_basics)
# add_debug_test(byte_stream_capacity)
# add_debug_test(byte_stream_one_write)
# add_debug_test(byte_stream_two_writes)
# add_debug_test(byte_stream_many_writes)
# add_debug_test(byte_stream_stress_test)

# add_debug_test(reassembler_single)
# add_debug_test(reassembler_cap)
# add_debug_test(reassembler_seq)
# add_debug_test(reassembler_dup)
# add_debug_test(reassembler_holes)
# add_debug_test(reassembler_overlapping)
# add_debug_test(reassembler_win)

# add_debug_test(wrapping_integers_cmp)
# add_debug_test(wrapping_integers_wrap)
# add_debug_test(wrapping_integers_unwrap)
# add_debug_test(wrapping_integers_roundtrip)
# add_debug_test(wrapping_integers_extra)

# add_debug_test(recv_connect)
# add_debug_test(recv_transmit)
# add_debug_test(recv_window)
# add_debug_test(recv_reorder)
# add_debug_test(recv_reorder_more)
# add_debug_test(recv_close)
# add_debug_test(recv_special)

# add_debug_test(send_connect)
# add_debug_test(send_transmit)
# add_debug_test(send_retx)
# add_debug_test(send_window)
# add_debug_test(send_ack)
# add_debug_test(send_close)
# add_debug_test(send_extra)

# add_debug_test(net_interface)

# add_debug_test(router)


add_custom_target (check0 COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --stop-on-failure --timeout 12 -R 'webget|^byte_stream_')

add_custom_target (check_webget COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --timeout 12 -R 'webget')

add_custom_target (check1 COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --stop-on-failure --timeout 12 -R '^byte_stream_|^reassembler_')

add_custom_target (check2 COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --stop-on-failure --timeout 12 -R '^byte_stream_|^reassembler_|^wrapping|^recv')

add_custom_target (check3 COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --stop-on-failure --timeout 12 -R '^byte_stream_|^reassembler_|^wrapping|^recv|^send')

add_custom_target (check5 COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --stop-on-failure --timeout 12 -R '^net_interface')

add_custom_target (check6 COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --stop-on-failure --timeout 12 -R '^net_interface|^router')

###

add_custom_target (speed COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --timeout 12 -R '_speed_test')

# # 添加一个调试目标以简化启动 GDB 的过程
# add_custom_target(debug_check6
#   COMMAND gdb --args ${CMAKE_CTEST_COMMAND}  --output-on-failure --stop-on-failure --timeout 12 -R '^net_interface|^router'
#   COMMENT "Starting GDB for debugging check6 tests"
# )

set(compile_name_opt "compile with optimization")
add_test(NAME ${compile_name_opt}
  COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" -t speed_testing)

macro (stest name)
  add_test(NAME ${name} COMMAND ${name})
  set_property(TEST ${name} PROPERTY FIXTURES_REQUIRED compile_opt)
endmacro (stest)

set_property(TEST ${compile_name_opt} PROPERTY TIMEOUT 0)
set_tests_properties(${compile_name_opt} PROPERTIES FIXTURES_SETUP compile_opt)

stest(byte_stream_speed_test)
stest(reassembler_speed_test)
