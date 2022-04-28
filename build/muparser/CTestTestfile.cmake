# CMake generated Testfile for 
# Source directory: H:/Data/C++/CBG/muparser
# Build directory: H:/Data/C++/CBG/build/muparser
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(ParserTest "H:/Data/C++/CBG/bin/Debug/t_ParserTest.exe")
  set_tests_properties(ParserTest PROPERTIES  _BACKTRACE_TRIPLES "H:/Data/C++/CBG/muparser/CMakeLists.txt;154;add_test;H:/Data/C++/CBG/muparser/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(ParserTest "H:/Data/C++/CBG/bin/Release/t_ParserTest.exe")
  set_tests_properties(ParserTest PROPERTIES  _BACKTRACE_TRIPLES "H:/Data/C++/CBG/muparser/CMakeLists.txt;154;add_test;H:/Data/C++/CBG/muparser/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(ParserTest "H:/Data/C++/CBG/bin/MinSizeRel/t_ParserTest.exe")
  set_tests_properties(ParserTest PROPERTIES  _BACKTRACE_TRIPLES "H:/Data/C++/CBG/muparser/CMakeLists.txt;154;add_test;H:/Data/C++/CBG/muparser/CMakeLists.txt;0;")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(ParserTest "H:/Data/C++/CBG/bin/RelWithDebInfo/t_ParserTest.exe")
  set_tests_properties(ParserTest PROPERTIES  _BACKTRACE_TRIPLES "H:/Data/C++/CBG/muparser/CMakeLists.txt;154;add_test;H:/Data/C++/CBG/muparser/CMakeLists.txt;0;")
else()
  add_test(ParserTest NOT_AVAILABLE)
endif()
