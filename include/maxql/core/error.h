#pragma once

#include <stdio.h>
#include <stdarg.h>

typedef enum {
    ERR_OK = 0,
    ERR_INTERNAL,
    ERR_LEXER,
    ERR_PARSER,
    ERR_VALIDATOR,
    ERR_ANALYZER,
    ERR_PLANNER,
    ERR_EXECUTOR,
    ERR_STORAGE,
    ERR_MEMORY,
    ERR_INDEX,
} ErrorCode;

typedef enum {
    ERR_SEVERITY_NORMAL,
    ERR_SEVERITY_FATAL,
} ErrorSeverity;

typedef struct {
    ErrorCode code;
    ErrorSeverity severity;
    char msg[256];
} Error;

[[maybe_unused]] static Error error_ok() { return (Error){ERR_OK, ERR_SEVERITY_NORMAL, ""}; }

[[maybe_unused]] static __attribute__((format(printf, 2, 3)))
Error error_fmt(const ErrorCode code, const char* msg, ...)
{
    Error e = {.code = code, .severity = ERR_SEVERITY_NORMAL};
    va_list args;
    va_start(args, msg);
    vsnprintf(e.msg, sizeof(e.msg), msg, args);
    va_end(args);
    return e;
}

[[maybe_unused]] static __attribute__((format(printf, 2, 3)))
Error error_fatal_fmt(const ErrorCode code, const char* msg, ...)
{
    Error e = {.code = code, .severity = ERR_SEVERITY_FATAL};
    va_list args;
    va_start(args, msg);
    vsnprintf(e.msg, sizeof(e.msg), msg, args);
    va_end(args);
    return e;
}

[[maybe_unused]] static bool error_is_ok(const Error e) { return e.code == ERR_OK; }
[[maybe_unused]] static bool error_is_fatal(const Error e) { return e.severity != ERR_SEVERITY_NORMAL; }

void handle_fatal_error(Error error);
