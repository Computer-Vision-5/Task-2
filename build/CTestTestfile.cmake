# CMake generated Testfile for 
# Source directory: C:/Users/shams/CLionProjects/untitled4
# Build directory: C:/Users/shams/CLionProjects/untitled4/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[backend_smoke_test]=] "C:/Users/shams/CLionProjects/untitled4/build/Debug/backend_smoke_test.exe")
  set_tests_properties([=[backend_smoke_test]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/shams/CLionProjects/untitled4/CMakeLists.txt;28;add_test;C:/Users/shams/CLionProjects/untitled4/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[backend_smoke_test]=] "C:/Users/shams/CLionProjects/untitled4/build/Release/backend_smoke_test.exe")
  set_tests_properties([=[backend_smoke_test]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/shams/CLionProjects/untitled4/CMakeLists.txt;28;add_test;C:/Users/shams/CLionProjects/untitled4/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[backend_smoke_test]=] "C:/Users/shams/CLionProjects/untitled4/build/MinSizeRel/backend_smoke_test.exe")
  set_tests_properties([=[backend_smoke_test]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/shams/CLionProjects/untitled4/CMakeLists.txt;28;add_test;C:/Users/shams/CLionProjects/untitled4/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[backend_smoke_test]=] "C:/Users/shams/CLionProjects/untitled4/build/RelWithDebInfo/backend_smoke_test.exe")
  set_tests_properties([=[backend_smoke_test]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/shams/CLionProjects/untitled4/CMakeLists.txt;28;add_test;C:/Users/shams/CLionProjects/untitled4/CMakeLists.txt;0;")
else()
  add_test([=[backend_smoke_test]=] NOT_AVAILABLE)
endif()
