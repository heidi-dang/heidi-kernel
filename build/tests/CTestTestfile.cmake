# CMake generated Testfile for 
# Source directory: /home/nguyen/heidi-kernel/tests
# Build directory: /home/nguyen/heidi-kernel/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[heidi-kernel-tests]=] "/home/nguyen/heidi-kernel/build/tests/heidi-kernel-tests")
set_tests_properties([=[heidi-kernel-tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/nguyen/heidi-kernel/tests/CMakeLists.txt;22;add_test;/home/nguyen/heidi-kernel/tests/CMakeLists.txt;0;")
subdirs("../_deps/catch2-build")
