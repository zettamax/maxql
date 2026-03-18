#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <termios.h>
#include <maxql/constants.h>
#include <maxql/cli/repl.h>

#define PROMPT_STRING "maxql> "
#define HISTORY_FILE ".maxql_history"

#define MAX_INPUT_LEN 8192
#define HISTORY_CAPACITY 100

#define KEY_OFFSET 1000
#define KEY_UP     (KEY_OFFSET + 'A')
#define KEY_DOWN   (KEY_OFFSET + 'B')
#define KEY_RIGHT  (KEY_OFFSET + 'C')
#define KEY_LEFT   (KEY_OFFSET + 'D')
#define KEY_DELETE (KEY_OFFSET + '~')
#define KEY_CTRL_A ('A' - 64)
#define KEY_CTRL_D ('D' - 64)
#define KEY_CTRL_E ('E' - 64)
#define KEY_CTRL_L ('L' - 64)
#define KEY_CTRL_R ('R' - 64)
#define KEY_CTRL_W ('W' - 64)
#define KEY_CTRL_LEFT  (KEY_OFFSET + 200)
#define KEY_CTRL_RIGHT (KEY_OFFSET + 201)

static struct termios orig;

static void history_load(char history[][MAX_INPUT_LEN],
                         size_t* count,
                         size_t* write_pos)
{
    FILE* f = fopen(HISTORY_FILE, "r");
    if (!f)
        return;

    char line[MAX_INPUT_LEN];
    while (fgets(line, sizeof(line), f) && *count < HISTORY_CAPACITY) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        if (strlen(line) == 0)
            continue;

        strcpy(history[*write_pos], line);
        *write_pos = (*write_pos + 1) % HISTORY_CAPACITY;
        (*count)++;
    }
    fclose(f);
}

static void history_save(char history[][MAX_INPUT_LEN],
                         size_t count,
                         size_t write_pos)
{
    if (count == 0)
        return;

    FILE* f = fopen(HISTORY_FILE, "w");
    if (!f)
        return;

    size_t start = (write_pos - count + HISTORY_CAPACITY) % HISTORY_CAPACITY;

    for (size_t i = 0; i < count; i++) {
        size_t idx = (start + i) % HISTORY_CAPACITY;
        fprintf(f, "%s\n", history[idx]);
    }
    fclose(f);
}

static uint16_t read_key(void)
{
    uint8_t c;
    if (read(STDIN_FILENO, &c, 1) != 1)
        return 0;

    if (c != 27) // not ESC
        return c;

    // ESC sequence
    uint8_t seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1)
        return 27;
    if (seq[0] != '[')
        return 27;

    if (read(STDIN_FILENO, &seq[1], 1) != 1)
        return 27;

    // Arrows: ESC [ A/B/C/D
    if (seq[1] >= 'A' && seq[1] <= 'D')
        return KEY_OFFSET + seq[1];

    // Delete/Home/End/Ctrl+key: ESC [ 3 ~
    if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1)
            return 27;
        if (seq[2] == '~' && seq[1] == '3')
            return KEY_DELETE;
        // Ctrl+Arrow: ESC [ 1 ; 5 C/D
        if (seq[2] == ';' && seq[1] == '1') {
            uint8_t mod, arrow;
            if (read(STDIN_FILENO, &mod, 1) != 1) return 27;
            if (read(STDIN_FILENO, &arrow, 1) != 1) return 27;
            if (mod == '5' && arrow == 'C') return KEY_CTRL_RIGHT;
            if (mod == '5' && arrow == 'D') return KEY_CTRL_LEFT;
        }
    }

    return 27;
}

static void restore_terminal(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

static void signal_handler(const int sig)
{
    restore_terminal();
    write(STDOUT_FILENO, BYE_MSG, sizeof(BYE_MSG) - 1);
    _exit(sig + 128);
}

static bool is_duplicate_record(const char* buf,
                         const char (*history)[MAX_INPUT_LEN],
                         size_t history_count,
                         size_t history_write)
{
    if (history_count > 0) {
        size_t last_idx = (history_write - 1 + HISTORY_CAPACITY) % HISTORY_CAPACITY;
        if (strcmp(buf, history[last_idx]) == 0) {
            return true;
        }
    }

    return false;
}

static size_t word_left(const char* buf, size_t pos)
{
    if (pos == 0) return 0;
    while (pos > 0 && buf[pos - 1] == ' ') pos--;
    while (pos > 0 && buf[pos - 1] != ' ') pos--;
    return pos;
}

static size_t word_right(const char* buf, size_t buf_len, size_t pos)
{
    while (pos < buf_len && buf[pos] != ' ') pos++;
    while (pos < buf_len && buf[pos] == ' ') pos++;
    return pos;
}

void repl_run(ReplExecuteFn execute_fn, AppContext* app_ctx)
{
    if (!isatty(STDIN_FILENO)) {
        char* line = nullptr;
        size_t len = 0;
        ssize_t read;

        while ((read = getline(&line, &len, stdin)) != -1) {
            if (line[read - 1] == '\n')
                line[read - 1] = '\0';

            execute_fn(line, app_ctx);
        }

        free(line);
        return;
    }

    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig);
    atexit(restore_terminal);
    signal(SIGINT, signal_handler);
    raw = orig;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    char buf[MAX_INPUT_LEN] = {};
    size_t buf_len = 0;
    size_t cursor_pos = 0;

    char history[HISTORY_CAPACITY][MAX_INPUT_LEN] = {};
    size_t history_count = 0;
    size_t history_write = 0;
    history_load(history, &history_count, &history_write);

    char saved_input[MAX_INPUT_LEN] = {};
    size_t browse_offset = 0;

    bool should_exit = false;

    while (true) {
        printf("%s", PROMPT_STRING);
        fflush(stdout);

        for (uint16_t key = read_key(); key != 10 && key != 13; key = read_key()) {
            if (key == 0)
                continue;

            if (key == KEY_CTRL_D && buf_len == 0) {
                should_exit = true;
                break;
            }

            if (key == KEY_CTRL_L) {
                printf("\x1b[2J\x1b[H");
                goto redraw;
            }

            if (key == KEY_UP && browse_offset < history_count) {
                if (browse_offset == 0)
                    strcpy(saved_input, buf); // save current input

                browse_offset++;

                size_t idx = (history_write - browse_offset + HISTORY_CAPACITY) % HISTORY_CAPACITY;
                strcpy(buf, history[idx]);
                buf_len = cursor_pos = strlen(buf);
                goto redraw;
            }

            if (key == KEY_DOWN && browse_offset > 0) {
                browse_offset--;

                if (browse_offset == 0) {
                    strcpy(buf, saved_input);
                } else {
                    size_t idx = (history_write - browse_offset + HISTORY_CAPACITY) %
                                 HISTORY_CAPACITY;
                    strcpy(buf, history[idx]);
                }
                buf_len = cursor_pos = strlen(buf);
                goto redraw;
            }

            // Ctrl+A: line start jump
            if (key == KEY_CTRL_A && cursor_pos > 0) {
                cursor_pos = 0;
                goto redraw;
            }

            // Ctrl+E: line end jump
            if (key == KEY_CTRL_E && cursor_pos < buf_len) {
                cursor_pos = buf_len;
                goto redraw;
            }

            // Ctrl+W: remove word
            if (key == KEY_CTRL_W && cursor_pos > 0) {
                size_t new_pos = word_left(buf, cursor_pos);
                memmove(&buf[new_pos], &buf[cursor_pos], buf_len - cursor_pos);
                buf_len -= cursor_pos - new_pos;
                cursor_pos = new_pos;
                buf[buf_len] = '\0';
                goto redraw;
            }

            // Ctrl+Left: left word jump
            if (key == KEY_CTRL_LEFT && cursor_pos > 0) {
                cursor_pos = word_left(buf, cursor_pos);
                goto redraw;
            }

            // Ctrl+Right: right word jump
            if (key == KEY_CTRL_RIGHT && cursor_pos < buf_len) {
                cursor_pos = word_right(buf, buf_len, cursor_pos);
                goto redraw;
            }

            // Ctrl+R: reverse search
            if (key == KEY_CTRL_R) {
                char search_buf[256] = {};
                size_t search_len = 0;
                size_t match_idx = SIZE_MAX;

                printf("\r\x1b[K(reverse-search)`': ");
                fflush(stdout);

                uint8_t skey;
                while (read(STDIN_FILENO, &skey, 1) == 1) {
                    if (skey == 10 || skey == 13) {
                        if (match_idx != SIZE_MAX) {
                            strcpy(buf, history[match_idx]);
                            buf_len = cursor_pos = strlen(buf);
                        }
                        goto redraw;
                    }
                    if (skey == 27) {
                        goto redraw;
                    }
                    if (skey == 127 && search_len > 0) {
                        search_buf[--search_len] = '\0';
                    } else if (skey == KEY_CTRL_R) {
                        // repeat Ctrl+R: nothing to change, search_from handles it below
                    } else if (skey >= 32 && skey < 127 && search_len < 255) {
                        search_buf[search_len++] = (char)skey;
                        search_buf[search_len] = '\0';
                    } else {
                        continue;
                    }

                    size_t search_from = 0;
                    if (skey == KEY_CTRL_R && match_idx != SIZE_MAX) {
                        for (size_t i = 0; i < history_count; i++) {
                            size_t idx = (history_write - 1 - i + HISTORY_CAPACITY) % HISTORY_CAPACITY;
                            if (idx == match_idx) {
                                search_from = i + 1;
                                break;
                            }
                        }
                    }

                    match_idx = SIZE_MAX;
                    if (search_len > 0) {
                        for (size_t i = search_from; i < history_count; i++) {
                            size_t idx = (history_write - 1 - i + HISTORY_CAPACITY) % HISTORY_CAPACITY;
                            if (strstr(history[idx], search_buf)) {
                                match_idx = idx;
                                break;
                            }
                        }
                    }

                    const char* display = (match_idx != SIZE_MAX) ? history[match_idx] : "";
                    printf("\r\x1b[K(reverse-search)`%s': %s", search_buf, display);
                    fflush(stdout);
                }
            }

            if (key == KEY_RIGHT && cursor_pos < buf_len) {
                cursor_pos++;
                while (cursor_pos < buf_len && (buf[cursor_pos] & 0xC0) == 0x80) {
                    cursor_pos++;
                }
                printf("\x1b[C");
                fflush(stdout);
                continue;
            }

            if (key == KEY_LEFT && cursor_pos > 0) {
                do {
                    cursor_pos--;
                } while (cursor_pos > 0 && (buf[cursor_pos] & 0xC0) == 0x80);
                printf("\x1b[D");
                fflush(stdout);
                continue;
            }

            if (key == 127 && cursor_pos > 0) {
                size_t orig_pos = cursor_pos;
                do {
                    cursor_pos--;
                } while (cursor_pos > 0 && (buf[cursor_pos] & 0xC0) == 0x80);
                if (orig_pos != buf_len)
                    memmove(&buf[cursor_pos], &buf[orig_pos], buf_len - orig_pos);
                buf_len -= orig_pos - cursor_pos;
                buf[buf_len] = '\0';
                goto redraw;
            }

            if (key == KEY_DELETE && cursor_pos < buf_len) {
                size_t next_symbol_pos = cursor_pos;
                next_symbol_pos++;
                while (next_symbol_pos < buf_len && (buf[next_symbol_pos] & 0xC0) == 0x80) {
                    next_symbol_pos++;
                }
                memmove(&buf[cursor_pos], &buf[next_symbol_pos], buf_len - next_symbol_pos);
                buf_len -= next_symbol_pos - cursor_pos;
                buf[buf_len] = '\0';
                goto redraw;
            }

            if (key >= 32 && key < KEY_OFFSET && buf_len < MAX_INPUT_LEN - 1) {
                if (cursor_pos != buf_len)
                    memmove(&buf[cursor_pos + 1], &buf[cursor_pos], buf_len - cursor_pos);
                buf[cursor_pos] = (char)key;
                buf_len++;
                cursor_pos++;
                buf[buf_len] = '\0';
                goto redraw;
            }

            continue;

        redraw:
            printf("\r\x1b[K%s%s", PROMPT_STRING, buf);
            if (cursor_pos < buf_len) {
                size_t non_continuation_symbol_count = 0;
                for (size_t i = cursor_pos; i < buf_len; i++)
                    if ((buf[i] & 0xC0) != 0x80)
                        non_continuation_symbol_count++;
                printf("\x1b[%zuD", non_continuation_symbol_count);
            }
            fflush(stdout);
        }

        if (should_exit || strcmp(buf, "exit") == 0)
            break;

        printf("\r\n");

        if (buf_len == 0)
            continue;

        if (!is_duplicate_record(buf, history, history_count, history_write)) {
            strcpy(history[history_write], buf);
            history_write = (history_write + 1) % HISTORY_CAPACITY;
            if (history_count < HISTORY_CAPACITY)
                history_count++;
            history_save(history, history_count, history_write);
        }

        execute_fn(buf, app_ctx);

        buf[0] = '\0';
        buf_len = 0;
        cursor_pos = 0;
        browse_offset = 0;
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}