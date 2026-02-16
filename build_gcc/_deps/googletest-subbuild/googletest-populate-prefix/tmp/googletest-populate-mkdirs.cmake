# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/app/build_gcc/_deps/googletest-src"
  "/app/build_gcc/_deps/googletest-build"
  "/app/build_gcc/_deps/googletest-subbuild/googletest-populate-prefix"
  "/app/build_gcc/_deps/googletest-subbuild/googletest-populate-prefix/tmp"
  "/app/build_gcc/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp"
  "/app/build_gcc/_deps/googletest-subbuild/googletest-populate-prefix/src"
  "/app/build_gcc/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/app/build_gcc/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/app/build_gcc/_deps/googletest-subbuild/googletest-populate-prefix/src/googletest-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
