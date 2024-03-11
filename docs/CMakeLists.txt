set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(OptionDocumentationGenerator OptionDocumentationGenerator.cpp)

add_custom_target(gen-options
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/OptionDocumentationGenerator
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/OptionDocumentationGenerator.py --insights ${CMAKE_CURRENT_BINARY_DIR}/../insights --cxx ${CMAKE_CXX_COMPILER}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generate command line options docu" VERBATIM
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/OptionDocumentationGenerator ${CMAKE_CURRENT_SOURCE_DIR}/OptionDocumentationGenerator.py
)

