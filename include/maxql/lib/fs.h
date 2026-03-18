#pragma once

#include <stdio.h>
#include <maxql/core/error.h>

Error open_file(const char* file_name,
                const char* mode,
                FILE** file,
                ErrorCode code,
                const char* msg);
Error delete_file(const char* file_name, ErrorCode code, const char* msg);
bool check_file_exists(const char* filename);
