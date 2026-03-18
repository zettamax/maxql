#include <maxql/lib/fs.h>
#include <maxql/core/error.h>

Error open_file(const char* file_name,
                const char* mode,
                FILE** file,
                ErrorCode code,
                const char* msg)
{
    *file = fopen(file_name, mode);
    if (!*file)
        return error_fatal_fmt(code, msg);

    return error_ok();
}

Error delete_file(const char* file_name, ErrorCode code, const char* msg)
{
    if (remove(file_name) == 0)
        return error_ok();
    return error_fatal_fmt(code, msg);
}

bool check_file_exists(const char* filename)
{
    FILE* file = fopen(filename, "r");
    bool exists = !!file;
    if (exists)
        fclose(file);

    return exists;
}
