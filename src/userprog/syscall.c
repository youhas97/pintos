#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/inode.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void halt(void){
    power_off();
}

bool create (const char *file, unsigned initial_size){
    return filesys_create(file, initial_size);
}

int open (const char *file){
    struct thread *t = thread_current();
    struct file *file_ptr = filesys_open(file);

    if (file_ptr) {
        int i;
        for (i = 0; i<MAX_FILES; ++i) {
            if (t->files[i] == NULL) {
                t->files[i] = file_ptr;
                return i+OFFSET;
            }
        }
    }
    file_close(file_ptr);
    return -1;
}

void close(int fd) {
    struct thread *t = thread_current();

    if (fd >= OFFSET && fd < MAX_FILES + OFFSET) {
        struct file *file_ptr = t->files[fd-OFFSET];

        if (file_ptr) {
            file_close(file_ptr);
            t->files[fd-OFFSET] = NULL;
        }
    }
}

int read (int fd, void *buffer, unsigned size){
    if (fd == 0) {
        int bytes_read;
        for (bytes_read = 0;  bytes_read < size; ++bytes_read) {
            char c = input_getc();
            *(char*)buffer = c;
            (char*)buffer++;
        }
        return bytes_read;
    }

    struct thread *t = thread_current();
    if (fd >= OFFSET && fd < MAX_FILES + OFFSET) {
        struct file *file_ptr = t->files[fd-OFFSET];

        if (file_ptr)
            return file_read(file_ptr, buffer, size);
    }
    return -1;
}

int write (int fd, const void *buffer, unsigned size){
    if (fd == 1) {
        putbuf((char*)buffer, size);
        return size;
    }

    struct thread *t = thread_current();
    if (fd >= OFFSET && fd < MAX_FILES + OFFSET) {
        struct file *file_ptr = t->files[fd-OFFSET];

        if (file_ptr)
            return file_write(file_ptr, buffer, size);
    }
    return -1;

}

tid_t exec (const char *cmd_line) {
    return process_execute(cmd_line);
}

void exit (int status){
    thread_exit();
}

static void
syscall_handler (struct intr_frame *f UNUSED)
{
    int *arg = (int*)f->esp;

    switch(arg[0]){
        case SYS_HALT:
            halt();
        case SYS_EXIT:
            exit((int)arg[1]);
            break;
        case SYS_CREATE:
            f->eax = create((char*)arg[1], (unsigned)arg[2]);
            break;
        case SYS_OPEN:
            f->eax = open((char*)arg[1]);
            break;
        case SYS_READ:
            f->eax = read((int)arg[1], (void*)arg[2], (unsigned)arg[3]);
            break;
        case SYS_WRITE:
            f->eax = write((int)arg[1], (void*)arg[2], (unsigned)arg[3]);
            break;
        case SYS_CLOSE:
            close((int)arg[1]);
            break;
        case SYS_EXEC:
            exec((char*)arg[1]);
            break;
        default:
            printf ("system call! down here\n");
    }
}
