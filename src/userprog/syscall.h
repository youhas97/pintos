#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
void syscall_init (void);
void close(int fd);
#endif /* userprog/syscall.h */


#define MAX_FILES 128
#define OFFSET 2
