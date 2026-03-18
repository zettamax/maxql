#include <maxql/view/output.h>
#include <stdio.h>
#include <time.h>
#include <maxql/core/error.h>
#include <maxql/types.h>

void print_error(Error error)
{
    printf("Error: %s\n", error.msg);
}

void print_time(TimeDelta td)
{
    long sec = td.end.tv_sec - td.start.tv_sec;
    long nsec = td.end.tv_nsec - td.start.tv_nsec;

    if (nsec < 0) {
        sec--;
        nsec += 1000000000L;
    }

    if (sec > 0) {
        long tenths = nsec / 100000000L;
        printf("Time: %ld.%ld sec\n", sec, tenths);
    } else {
        printf("Time: %ld msec\n", nsec / 1000000L);
    }
}

void print_index_info(bool used, const char* name)
{
    if (used)
        printf("Index used: %s\n", name);
    else
        printf("No index used\n");
}

void print_affected_rows(size_t count)
{
    printf("Affected rows: %zu\n", count);
}

void print_ok(void)
{
    printf("OK\n");
}
