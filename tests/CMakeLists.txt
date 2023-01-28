
mypower_scan_test(PATTERN "test_scanner_*" LIBRARIES scanner)
mypower_scan_test(PATTERN "test_expr*" LIBRARIES dsl)

if (OpenMP_CXX_FOUND)
    set(LIBS OpenMP::OpenMP_CXX)
endif()

mypower_scan_test(PATTERN "test_command*" 
    SOURCES $<TARGET_OBJECTS:mypower> LIBRARIES dsl tui scanner Boost::program_options Boost::json ${LIBS})
