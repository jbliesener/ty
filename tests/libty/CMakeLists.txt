# TyTools - public domain
# Niels Martignène <niels.martignene@gmail.com>
# https://neodd.com/tytools

# This software is in the public domain. Where that dedication is not
# recognized, you are granted a perpetual, irrevocable license to copy,
# distribute, and modify this file as you see fit.

# See the LICENSE file for more details.

add_executable(test_libty test_libty.c
                          test_optline.c)
target_link_libraries(test_libty libhs libty)
add_test(NAME libty COMMAND test_libty)
