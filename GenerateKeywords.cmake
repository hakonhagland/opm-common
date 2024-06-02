set(genkw_SOURCES opm/json/JsonObject.cpp
                  opm/input/eclipse/Deck/DeckTree.cpp
                  opm/input/eclipse/Deck/DeckValue.cpp
                  opm/input/eclipse/Deck/Deck.cpp
                  opm/input/eclipse/Deck/DeckView.cpp
                  opm/input/eclipse/Deck/DeckItem.cpp
                  opm/input/eclipse/Deck/DeckKeyword.cpp
                  opm/input/eclipse/Deck/DeckRecord.cpp
                  opm/input/eclipse/Deck/DeckOutput.cpp
                  opm/input/eclipse/Deck/UDAValue.cpp
                  opm/input/eclipse/Generator/KeywordGenerator.cpp
                  opm/input/eclipse/Generator/KeywordLoader.cpp
                  opm/input/eclipse/Schedule/UDQ/UDQEnums.cpp
                  opm/input/eclipse/Parser/createDefaultKeywordList.cpp
                  opm/input/eclipse/Parser/ErrorGuard.cpp
                  opm/input/eclipse/Parser/ParseContext.cpp
                  opm/input/eclipse/Parser/ParserEnums.cpp
                  opm/input/eclipse/Parser/ParserItem.cpp
                  opm/input/eclipse/Parser/ParserKeyword.cpp
                  opm/input/eclipse/Parser/ParserRecord.cpp
                  opm/input/eclipse/Parser/raw/RawKeyword.cpp
                  opm/input/eclipse/Parser/raw/RawRecord.cpp
                  opm/input/eclipse/Parser/raw/StarToken.cpp
                  opm/input/eclipse/Units/Dimension.cpp
                  opm/input/eclipse/Units/UnitSystem.cpp
                  opm/common/utility/OpmInputError.cpp
                  opm/common/utility/shmatch.cpp
                  opm/common/utility/String.cpp
                  opm/common/OpmLog/OpmLog.cpp
                  opm/common/OpmLog/Logger.cpp
                  opm/common/OpmLog/StreamLog.cpp
                  opm/common/OpmLog/LogBackend.cpp
                  opm/common/OpmLog/LogUtil.cpp
)
if(NOT cjson_FOUND)
  list(APPEND genkw_SOURCES ${cjson_SOURCE_DIR}/cJSON.c)
endif()
add_executable(genkw ${genkw_SOURCES})

target_link_libraries(genkw ${opm-common_LIBRARIES})

# Generate keyword list
include(opm/input/eclipse/share/keywords/keyword_list.cmake)
string(REGEX REPLACE "([^;]+)" "${PROJECT_SOURCE_DIR}/opm/input/eclipse/share/keywords/\\1" keyword_files "${keywords}")
configure_file(opm/input/eclipse/keyword_list.argv.in keyword_list.argv)

# Generate keyword source

set( genkw_argv keyword_list.argv
  ${PROJECT_BINARY_DIR}/tmp_gen/ParserKeywords
  ${PROJECT_BINARY_DIR}/tmp_gen/ParserInit.cpp
  ${PROJECT_BINARY_DIR}/tmp_gen/include/
  opm/input/eclipse/Parser/ParserKeywords
  ${PROJECT_BINARY_DIR}/tmp_gen/TestKeywords.cpp)

foreach (name A B C D E F G H I J K L M N O P Q R S T U V W X Y Z)
  list(APPEND _tmp_output ${PROJECT_BINARY_DIR}/tmp_gen/ParserKeywords/${name}.cpp
                          ${PROJECT_BINARY_DIR}/tmp_gen/include/opm/input/eclipse/Parser/ParserKeywords/${name}.hpp
                          ${PROJECT_BINARY_DIR}/tmp_gen/ParserKeywords/ParserInit${name}.cpp
                          ${PROJECT_BINARY_DIR}/tmp_gen/ParserKeywords/Builtin${name}.cpp
                          ${PROJECT_BINARY_DIR}/tmp_gen/include/opm/input/eclipse/Parser/ParserKeywords/ParserInit${name}.hpp)
  list(APPEND _target_output ${PROJECT_BINARY_DIR}/ParserKeywords/${name}.cpp
                             ${PROJECT_BINARY_DIR}/include/opm/input/eclipse/Parser/ParserKeywords/${name}.hpp
                             ${PROJECT_BINARY_DIR}/ParserKeywords/ParserInit${name}.cpp
                             ${PROJECT_BINARY_DIR}/ParserKeywords/Builtin${name}.cpp
                             ${PROJECT_BINARY_DIR}/include/opm/input/eclipse/Parser/ParserKeywords/ParserInit${name}.hpp)
endforeach()

foreach(name TestKeywords.cpp ParserInit.cpp)
  list(APPEND _target_output ${PROJECT_BINARY_DIR}/${name})
  list(APPEND _tmp_output ${PROJECT_BINARY_DIR}/tmp_gen/${name})
endforeach()

list(APPEND _target_output ${PROJECT_BINARY_DIR}/include/opm/input/eclipse/Parser/ParserKeywords/Builtin.hpp)
list(APPEND _tmp_output ${PROJECT_BINARY_DIR}/tmp_gen/include/opm/input/eclipse/Parser/ParserKeywords/Builtin.hpp)

set(GEN_DEPS ${_tmp_output})
if (OPM_ENABLE_EMBEDDED_PYTHON)
  list(APPEND genkw_argv ${PROJECT_BINARY_DIR}/tmp_gen/builtin_pybind11.cpp)
  list(APPEND _tmp_output ${PROJECT_BINARY_DIR}/tmp_gen/builtin_pybind11.cpp)
  list(APPEND _target_output ${PROJECT_BINARY_DIR}/python/cxx/builtin_pybind11.cpp)
  list(APPEND GEN_DEPS copy_python)
endif()

add_custom_command( OUTPUT
  ${_tmp_output}
  COMMAND genkw ${genkw_argv}
  DEPENDS genkw ${keyword_files} opm/input/eclipse/share/keywords/keyword_list.cmake)

# To avoid some rebuilds
add_custom_command(OUTPUT
  ${_target_output}
  DEPENDS ${GEN_DEPS}
  COMMAND ${CMAKE_COMMAND} -DBASE_DIR=${PROJECT_BINARY_DIR} -P ${PROJECT_SOURCE_DIR}/CopyHeaders.cmake)
