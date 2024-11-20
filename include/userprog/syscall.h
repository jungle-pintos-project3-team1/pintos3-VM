#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

/* === project2 - System Call === */
#include <stdbool.h>

typedef int pid_t;

void check_address(void *addr);
void halt(void);
void exit(int status);
pid_t fork(const char *thread_name);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned length);
int write(int fd, const void *buffer, unsigned length);
int tell(int fd);
void close(int fd);
int dup2(int oldfd, int newfd);

void syscall_init(void);
#endif /* userprog/syscall.h */
