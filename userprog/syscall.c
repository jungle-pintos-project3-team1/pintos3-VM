#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "userprog/process.c"

/** #Project 2: System Call */
#include "filesys/filesys.h"


void syscall_entry (void);
void syscall_handler (struct intr_frame *);
/* === project2 - System Call === */
struct lock filesys_lock;

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);

	/* === project2 - System Call : read & write 용 Lock 초기화=== */
	lock_init(&filesys_lock);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	int sys_number = f->R.rax; // rax에 있는 시스템 콜 번호 추출

	switch(sys_number)
	{
		case SYS_HALT:
			half();
			break;
		case SYS_EXIT:
			exit(f->R.rdi);
			break;
		case SYS_FORK:
			f->R.rax = fort(f->R.rdi);
			break;
		case SYS_EXEC:
			f->R.rax = exec(f->R.rdi);
			break;
		case SYS_WAIT:
			f->R.rax = process_wait(f->R.rdi);
			break;
		case SYS_CREATE :
			f->R.rax = create(f->R.rdi, f->R.rsi);
			break;
		case SYS_REMOVE:
			f->R.rax = remove(f->R.rdi);
			break;
		case SYS_OPEN:
			f->R.rax = open(f->R.rdi);
			break;
		case SYS_FILESIZE:
			f->R.rax = filesize(f->R.rdi);
			break;
			// 브레이크땐스 시작
		case SYS_READ:
			f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_SEEK:
			seek(f->R.rdi, f->R.rsi);
			break;
		case SYS_TELL:
			f->R.rax = tell(f->R.rdi);
			break;
		default:
			exit(-1);
		}
}

void check_address(void *addr) {
	/* 유저 영역이 아닌 경우 프로세스 종료 */
	if (is_kernel_vaddr(addr) || addr == NULL || pml4_get_page(thread_current() -> pml4, addr) == NULL)
		exit(-1);
}

/* 핀토스를 종료시키는 시스템 콜 */
void halt(void){
	power_off();
}

/* 현재 프로세스를 종료시키는 시스템 콜 */
void exit(int status){
	struct thread *t = thread_current();
	t->exit_status = status;
	printf("%s: exit(%d)\n", t->name, t->exit_status); // Process Tremination Message
	thread_eixt();
}

/* 파일을 생성하는 시스템 콜 */
bool create(const char *file, unsigned initial_size){
	check_address(file);

	return filesys_create(file, initial_size);
}

/* 파일을 삭제하는 시스템 콜 */
bool remove(const char *file){
	check_address(file);

	return filesys_remove(file);
}

/* 파일을 여는 시스템 콜 */
int open(const char *file){
	check_address(file);

	struct file *newfile = filesys_open(file);

	if (newfile == NULL)
		return -1;

	int fd = process_add_file(newfile);

	if (fd == -1)
		file_close(newfile);

	return fd;
}

/* 파일의 크기를 알려주는 시스템 콜 */
int filesize(int fd) {
	struct file *file = process_get_file(fd);

	if (file == NULL)
		return -1;

	return file_length(file);
}

/* 열린 파일의 데이터를 읽는 시스템 콜 */
int read(int fd, void *buffer, unsigned length)
{
	check_address(buffer);

	if (fd == 0){
		int i =0;
		char c;
		unsigned char *buf = buffer;

		for (; i < length; i++){
			c = input_getc();		// 키보드 입력 한 글자 읽기
			*buf++ = c;				// 입력받은 문자를 버퍼에 저장
			if(c == '\0')			// 널 종단 문자일 경우 중단
				break;
		}

		return i;
	}

	if(fd < 3)
		return -1;

	struct file *file = process_get_file(fd);
	off_t bytes = -1;

	if (file == NULL)
		return -1;

	lock_acquire(&filesys_lock);
	bytes = file_read(file, buffer, length);
	lock_release(&filesys_lock);

	return bytes;
}

/* 열린 파일에 데이터를 쓰는 시스템 콜 */
int write(int fd, const void *buffer, unsigned length)
{
	check_address(buffer);

	off_t bytes = -1;

	if (fd <= 0)
		return -1;
	
	if (fd < 3){
		putbuf(buffer, length);
		return length;
	}

	struct file *file = process_get_file(fd);

	if (file == NULL)
		return -1;

	lock_acquire(&filesys_lock);
	bytes = file_write(file, buffer, length);
	lock_release(&filesys_lock);

	return bytes;
}