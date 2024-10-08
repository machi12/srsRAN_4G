
execute_process(
COMMAND git rev-parse --abbrev-ref HEAD
WORKING_DIRECTORY "/Users/machi/Desktop/Artifact/srsRAN_4G"
OUTPUT_VARIABLE GIT_BRANCH
OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
COMMAND git log -1 --format=%h
WORKING_DIRECTORY "/Users/machi/Desktop/Artifact/srsRAN_4G"
OUTPUT_VARIABLE GIT_COMMIT_HASH
OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Generating build_info.h")
configure_file(
  /Users/machi/Desktop/Artifact/srsRAN_4G/lib/include/srsran/build_info.h.in
  /Users/machi/Desktop/Artifact/srsRAN_4G/cmake-build-debug/lib/include/srsran/build_info.h
)
