#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <glib.h>

#include "colors.h"
#include "signals.h"
#include "test.h"

typedef struct TestListItem {
    Test *test;
    const char *name;
} TestListItem;

static GList *all_tests = NULL;

void register_test_(Test *test, const char *name)
{
    TestListItem *item = (TestListItem *)calloc(sizeof(TestListItem), 1);
    item->test = test;
    item->name = name;
    all_tests = g_list_append(all_tests, item);
}

static char *test_log_name_from_test_name(const char *test_name)
{
    char *output_name = g_strdup_printf("test_%s.log", test_name);
    char *result = strdup(output_name);
    g_free(output_name);
    return result;
}

static void redirect_test_output(const char *test_name)
{
    char *output_name = test_log_name_from_test_name(test_name);
    int fd = open(output_name, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd == -1)
        goto err;
    if (dup2(fd, STDOUT_FILENO) == -1)
        goto err;
    if (dup2(fd, STDERR_FILENO) == -1)
        goto err;

    close(fd);
    free(output_name);
    return;
err:
    fprintf(stderr, "tint2: Could not redirect test output to file name: %s\n", output_name);
    if (fd != -1)
        close(fd);
    free(output_name);
}

static void crash(int sig)
{
    kill(getpid(), SIGSEGV);
}

__attribute__((noreturn))
static void run_test_child(TestListItem *item)
{
    reset_signals();
    struct sigaction sa = {.sa_handler = crash};
    sigaction(SIGINT, &sa, 0);
    redirect_test_output(item->name);
    bool result = true;
    item->test(&result);
    exit(result ? EXIT_SUCCESS : EXIT_FAILURE);
}

static FILE *open_test_log(const char *test_name)
{
    char *output_name = test_log_name_from_test_name(test_name);
    FILE *log = fopen(output_name, "a");
    free(output_name);
    return log;
}

static Status run_test_parent(TestListItem *item, pid_t child)
{
    FILE *log = open_test_log(item->name);
    if (child == -1) {
        fprintf(log, "\n" "Test failed, fork failed\n");
        fclose(log);
        return FAILURE;
    }

    int child_status;
    pid_t ret_pid = waitpid(child, &child_status, 0);
    if (ret_pid != child) {
        fprintf(log, "\n" "Test failed, waitpid failed\n");
        fclose(log);
        return FAILURE;
    }
    if (WIFEXITED(child_status)) {
        int exit_status = WEXITSTATUS(child_status);
        if (exit_status == EXIT_SUCCESS) {
            fprintf(log, "\n" "Test succeeded.\n");
            fclose(log);
            return SUCCESS;
        } else {
            fprintf(log, "\n" "Test failed, exit status: %d.\n", exit_status);
            fclose(log);
            return FAILURE;
        }
    } else if (WIFSIGNALED(child_status)) {
        int signal = WTERMSIG(child_status);
        fprintf(log, "\n" "Test failed, child killed by signal: %d.\n", signal);
        fclose(log);
        return FAILURE;
    } else {
        fprintf(log, "\n" "Test failed, waitpid failed.\n");
        fclose(log);
        return FAILURE;
    }
}

static Status run_test(TestListItem *item)
{
    pid_t pid = fork();
    if (pid == 0)
        run_test_child(item);
    struct sigaction sa = {.sa_handler = SIG_IGN};
    sigaction(SIGINT, &sa, 0);
    return run_test_parent(item, pid);
}

void run_all_tests(bool verbose)
{
    fprintf(stdout, BLUE "tint2: Running %d tests..." RESET "\n", g_list_length(all_tests));
    size_t count = 0, succeeded = 0, failed = 0;
    for (GList *l = all_tests; l; l = l->next) {
        TestListItem *item = (TestListItem *)l->data;
        Status status = run_test(item);
        count++;
        fprintf(stdout, BLUE "tint2: Test " YELLOW "%s" BLUE ": ", item->name);
        if (status == SUCCESS) {
            fprintf(stdout, GREEN "succeeded" RESET "\n");
            succeeded++;
        } else {
            fprintf(stdout, RED "failed" RESET "\n");
            failed++;
            if (verbose) {
                char *log_name = test_log_name_from_test_name(item->name);
                FILE *log = fopen(log_name, "rt");
                if (log) {
                    char buffer[4096];
                    size_t num_read;
                    while ((num_read = fread(buffer, 1, sizeof(buffer), log)) > 0) {
                        fwrite(buffer, 1, num_read, stdout);
                    }
                    fclose(log);
                }
                free(log_name);
            }
        }
    }
    if (failed == 0)
        fprintf(stdout, BLUE "tint2: " GREEN "all %lu tests succeeded." RESET "\n", count);
    else
        fprintf(stdout, BLUE "tint2: " RED "%lu" BLUE " out of %lu tests " RED "failed." RESET "\n", failed, count);
}

#if 0
TEST(dummy) {
    int x = 2;
    int y = 2;
    ASSERT_EQUAL(x, y);
}

TEST(dummy_bad) {
    int x = 2;
    int y = 3;
    ASSERT_EQUAL(x, y);
}
#endif
