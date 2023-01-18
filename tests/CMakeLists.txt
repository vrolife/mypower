
mypower_scan_test(PATTERN "test_scanner_*" LIBRARIES scanner)
mypower_scan_test(PATTERN "test_expr*" LIBRARIES dsl)

mypower_scan_test(PATTERN "test_command*" 
    SOURCES $<TARGET_OBJECTS:mypower> LIBRARIES dsl tui scanner Boost::program_options Boost::json ZSTD::zstd)
