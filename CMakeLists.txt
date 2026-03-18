cmake_minimum_required(VERSION 3.21)
project(maxql C)

set(CMAKE_C_STANDARD 23)

find_program(IWYU_PATH NAMES include-what-you-use)

if(IWYU_PATH)
    set(CMAKE_C_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
endif()

add_compile_definitions(_FILE_OFFSET_BITS=64)
add_compile_definitions(_POSIX_C_SOURCE=200809L)
add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)

set(MAXQL_SOURCES
        src/core/strview.c
        src/core/dyn_array.c
        src/core/lexer.c
        src/parser/parser.c
        src/parser/schema_parser.c
        src/engine/engine.c
        src/schema/schema.c
        src/context/context.c
        src/storage/storage.c
        src/lib/fs.c
        src/serializer/serializer.c
        src/types/converter.c
        src/types/comparator.c
        src/cli/repl.c
        src/lib/mem.c
        src/core/error.c
        src/engine/ddl/executor.c
        src/engine/ddl/planner.c
        src/engine/ddl/validator.c
        src/engine/dml/validator.c
        src/engine/dml/analyzer.c
        src/engine/dml/planner.c
        src/engine/dml/executor.c
        src/engine/dal/executor.c
        src/database/database.c
        src/index/index.c
        src/index/btree.c
        src/scan/table_scan.c
        src/scan/index_scan.c
        src/types/mapper.c
        src/index/comparator.c
        src/view/printer.c
        src/types/formatter.c
        src/core/utf8.c
        src/view/output.c
        src/index/index_set.c
        src/index/index_io.c
        src/core/strbuf.c
        src/types/presentation.c
        src/types/key_compare.c
        src/parser/primitives.c
        src/schema/schema_io.c
)

set(MAXQL_HEADERS
        include/maxql/core/dyn_array.h
        include/maxql/core/strview.h
        include/maxql/core/error.h
        include/maxql/core/lexer.h
        include/maxql/parser/parser.h
        include/maxql/parser/ast.h
        include/maxql/engine/engine.h
        include/maxql/schema/schema.h
        include/maxql/context/context.h
        include/maxql/view/printer.h
        include/maxql/storage/storage.h
        include/maxql/lib/fs.h
        include/maxql/serializer/serializer.h
        include/maxql/limits.h
        include/maxql/types/converter.h
        include/maxql/types/comparator.h
        include/maxql/cli/repl.h
        include/maxql/lib/mem.h
        include/maxql/constants.h
        include/maxql/engine/ddl/types.h
        include/maxql/engine/ddl/validator.h
        include/maxql/engine/ddl/planner.h
        include/maxql/engine/ddl/executor.h
        include/maxql/engine/dml/validator.h
        include/maxql/engine/dml/analyzer.h
        include/maxql/engine/dml/planner.h
        include/maxql/engine/dml/executor.h
        include/maxql/engine/dal/executor.h
        include/maxql/database/database.h
        include/maxql/types.h
        include/maxql/index/index.h
        include/maxql/index/btree.h
        include/maxql/defaults.h
        include/maxql/scan/row_source.h
        include/maxql/scan/table_scan.h
        include/maxql/scan/index_scan.h
        include/maxql/types/mapper.h
        include/maxql/index/comparator.h
        include/maxql/types/formatter.h
        include/maxql/core/utf8.h
        include/maxql/view/output.h
        include/maxql/index/index_set.h
        include/maxql/index/index_io.h
        include/maxql/index/index_types.h
        include/maxql/types/types.h
        include/maxql/types/key_compare.h
        include/maxql/scan/scan_types.h
        include/maxql/parser/primitives.h
        include/maxql/engine/dml/types.h
        include/maxql/schema/schema_io.h
        include/maxql/parser/schema_parser.h
)

add_executable(maxql ${MAXQL_SOURCES} ${MAXQL_HEADERS} main.c)

target_include_directories(maxql PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)
