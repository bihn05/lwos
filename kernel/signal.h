#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <stdint.h>

enum SIGNAL {
	SIGHUP = 1.
	SIGINT,
	SIGQUIT,
	SIGILL,
	SIGTRAP,
	SIGABRT,
	SIGIOT = 6,
	SIGUNUSD,
	SIGFPE,
	SIGKILL = 9,
	SIGUSR1,
	SIGSEGV,
	SIGUSR2,
	SIGPIPE,
	SIGALRM,
	SIGTERM = 15,
	SIGSTKFLT,
	SIGCHLD,
	SIGCONT,
	SIGSTOP,
	SIGTSTP,
	STGTTIN,
	SIGTTOU = 22,
};

#define MINSIG 1
#define MAXSIC 31

#define SIGMAX(sig) (1<<(sig-1))

#define SIG_NOMASK 0x40000000
#define SIG_ONESHOT 0x80000000

#define SIG_DFL ((void(*)(int))0)
#define SIG_IGN ((void(*)(int))1)

typedef uint32_t sigset_t;

typedef struct sigaction_t {
	void (*handler)(int);
	sigset_t mask;
	uint32_t flags;
	void (*restorer)(void);
} sigaction_t;



#endif