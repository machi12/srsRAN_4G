# CMake generated Testfile for 
# Source directory: /Users/machi/Desktop/Artifact/srsRAN_4G/lib/test/pdcp
# Build directory: /Users/machi/Desktop/Artifact/srsRAN_4G/cmake-build-debug/lib/test/pdcp
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(pdcp_nr_test_tx "pdcp_nr_test_tx")
set_tests_properties(pdcp_nr_test_tx PROPERTIES  LABELS "nr;lib;pdcp" _BACKTRACE_TRIPLES "/Users/machi/Desktop/Artifact/srsRAN_4G/CMakeLists.txt;625;add_test;/Users/machi/Desktop/Artifact/srsRAN_4G/lib/test/pdcp/CMakeLists.txt;25;add_nr_test;/Users/machi/Desktop/Artifact/srsRAN_4G/lib/test/pdcp/CMakeLists.txt;0;")
add_test(pdcp_nr_test_rx "pdcp_nr_test_rx")
set_tests_properties(pdcp_nr_test_rx PROPERTIES  LABELS "nr;lib;pdcp" _BACKTRACE_TRIPLES "/Users/machi/Desktop/Artifact/srsRAN_4G/CMakeLists.txt;625;add_test;/Users/machi/Desktop/Artifact/srsRAN_4G/lib/test/pdcp/CMakeLists.txt;29;add_nr_test;/Users/machi/Desktop/Artifact/srsRAN_4G/lib/test/pdcp/CMakeLists.txt;0;")
add_test(pdcp_nr_test_discard_sdu "pdcp_nr_test_discard_sdu")
set_tests_properties(pdcp_nr_test_discard_sdu PROPERTIES  LABELS "nr;lib;pdcp" _BACKTRACE_TRIPLES "/Users/machi/Desktop/Artifact/srsRAN_4G/CMakeLists.txt;625;add_test;/Users/machi/Desktop/Artifact/srsRAN_4G/lib/test/pdcp/CMakeLists.txt;33;add_nr_test;/Users/machi/Desktop/Artifact/srsRAN_4G/lib/test/pdcp/CMakeLists.txt;0;")
add_test(pdcp_lte_test_rx "pdcp_lte_test_rx")
set_tests_properties(pdcp_lte_test_rx PROPERTIES  _BACKTRACE_TRIPLES "/Users/machi/Desktop/Artifact/srsRAN_4G/lib/test/pdcp/CMakeLists.txt;37;add_test;/Users/machi/Desktop/Artifact/srsRAN_4G/lib/test/pdcp/CMakeLists.txt;0;")
add_test(pdcp_lte_test_discard_sdu "pdcp_lte_test_discard_sdu")
set_tests_properties(pdcp_lte_test_discard_sdu PROPERTIES  _BACKTRACE_TRIPLES "/Users/machi/Desktop/Artifact/srsRAN_4G/lib/test/pdcp/CMakeLists.txt;41;add_test;/Users/machi/Desktop/Artifact/srsRAN_4G/lib/test/pdcp/CMakeLists.txt;0;")
add_test(pdcp_lte_test_status_report "pdcp_lte_test_status_report")
set_tests_properties(pdcp_lte_test_status_report PROPERTIES  _BACKTRACE_TRIPLES "/Users/machi/Desktop/Artifact/srsRAN_4G/lib/test/pdcp/CMakeLists.txt;45;add_test;/Users/machi/Desktop/Artifact/srsRAN_4G/lib/test/pdcp/CMakeLists.txt;0;")
