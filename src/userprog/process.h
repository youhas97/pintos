#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

// parent-child relationship
struct pc_status {
    int alive_count;
    int exit_status;

    bool exec_success;
    struct semaphore sema_wait;
    struct semaphore sema_exec;
    char *f_name;
    tid_t child_id;

    struct list_elem elem;      //list element

}

#endif /* userprog/process.h */
