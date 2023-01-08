
mypower_scan_test(PATTERN "test_scanner_*" LIBRARIES scanner)
mypower_scan_test(PATTERN "test_expr*" LIBRARIES dsl)
mypower_scan_test(PATTERN "test_command*" LIBRARIES dsl scanner tui commands)
