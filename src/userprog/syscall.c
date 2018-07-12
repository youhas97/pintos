#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "filesys/inode.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

static void syscall_handler (struct intr_frame *);

static bool
is_valid_ptr(const void *p) {
  struct thread *t = thread_current();
  // Checks if p is null, if p is a valid uaddr and if p is mapped in that order
  return ((p != NULL) && (is_user_vaddr(p) && (pagedir_get_page(t->pagedir, p) != NULL)));
}

static bool
is_valid_str(const char *s) {
  char c = *s;
  int i = 0;

  while(c != '\0') {
    if (is_valid_ptr(s + i)) {        //validate every letter in string.
	   c = *(s + i++);
       }
    else
	   return false;
  }
  return true;
}

static bool
is_valid_buf(const void *buf, size_t size) {
  unsigned i;
  for (i = 0; i <= size; ++i) {
    if(!is_valid_ptr(buf + i))
      return false;
  }
  return true;
}

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void halt(void){
    power_off();
}

bool create (const char *file, unsigned initial_size){
    /*
    why doesn't if(is_valid_ptr(file) && is_valid_str(file)) work?
    */
    if (is_valid_ptr(file) && is_valid_str(file))
        return filesys_create(file, initial_size);
    exit(-1);
    return false;
}

int open (const char *file){
    if (!is_valid_ptr(file) || !is_valid_str(file))
        exit(-1);
    else {
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
    }
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
    if(!is_valid_buf(buffer, size) || !is_valid_ptr(buffer))
        exit(-1);
    else {
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
    }
    return -1;
}

int write (int fd, const void *buffer, unsigned size){
    if (!is_valid_buf(buffer, size) || !is_valid_ptr(buffer))
        exit(-1);
    else {
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
    }
    return -1;
}

tid_t exec (const char *cmd_line) {
    if (!is_valid_ptr(cmd_line) || !is_valid_str(cmd_line)) {
        exit(-1);
        return -1;
    }
    return process_execute(cmd_line);

}

void exit (int status){
    struct thread *t = thread_current();
    t->parent_pcs->exit_status = status;
    printf("%s: exit(%d)\n", t->name, status);
    thread_exit();
}

int wait (tid_t pid) {
    return process_wait(pid);
}

void seek (int fd, unsigned position) {
    struct thread *t = thread_current();
    if (fd >= OFFSET && fd < MAX_FILES + OFFSET) {
        struct file *file_ptr = t->files[fd-OFFSET];

        if (file_ptr) {
            if (position > (unsigned)file_length(file_ptr))
                file_seek(file_ptr, file_length(file_ptr));
            else
                file_seek(file_ptr, position);
        }
    }
}

unsigned tell (int fd) {
    struct thread *t = thread_current();
    if (fd >= OFFSET && fd < MAX_FILES + OFFSET) {
        struct file *file_ptr = t->files[fd-OFFSET];

        if (file_ptr)
            return file_tell(file_ptr);
    }
    return -1; //I don't know what to return
}

int filesize (int fd) {
    struct thread *t = thread_current();
    if (fd >= OFFSET && fd < MAX_FILES + OFFSET) {
        struct file *file_ptr = t->files[fd-OFFSET];

        if (file_ptr)
            return file_length(file_ptr);
    }
    return -1;  //I don't know what to return
}

bool remove (const char *file_name) {
    if (is_valid_ptr(file_name) && is_valid_str(file_name)) {
        return filesys_remove(file_name);
    }
    exit(-1);
    return false;
}


static void
syscall_handler (struct intr_frame *f UNUSED)
{
    if(!is_valid_ptr(f->esp))
        exit(-1);
    else {
        int *arg = (int*)f->esp;

        switch(arg[0]){
            case SYS_HALT:
                halt();
                break;
            case SYS_EXIT:
                if(is_valid_ptr(&arg[1]))
                    exit((int)arg[1]);
                else
                    exit(-1);
                break;
            case SYS_CREATE:
                f->eax = create((char*)arg[1], (unsigned)arg[2]);
                break;
            case SYS_OPEN:
                if(is_valid_ptr(&arg[1]))
                    f->eax = open((char*)arg[1]);
                else
                    exit(-1);
                break;
            case SYS_READ:
                f->eax = read((int)arg[1], (void*)arg[2], (unsigned)arg[3]);
                break;
            case SYS_WRITE:
                f->eax = write((int)arg[1], (void*)arg[2], (unsigned)arg[3]);
                break;
            case SYS_CLOSE:
                if(is_valid_ptr(&arg[1]))
                    close((int)arg[1]);
                else
                    exit(-1);
                break;
            case SYS_EXEC:
                f->eax = exec((char*)arg[1]);
                break;
            case SYS_WAIT:
                if(is_valid_ptr(&arg[1]))
                    f->eax = wait((tid_t)arg[1]);
                else
                    exit(-1);
                break;
            case SYS_SEEK:
                seek((int)arg[1], (unsigned)arg[2]);
                break;
            case SYS_TELL:
                f->eax = tell((int)arg[1]);
                break;
            case SYS_FILESIZE:
                f->eax = filesize((int)arg[1]);
                break;
            case SYS_REMOVE:
                if(is_valid_ptr(&arg[1]))
                    f->eax = remove((char*)arg[1]);
                else
                    exit(-1);
                break;
            default:
                break;
        }
    }
}
