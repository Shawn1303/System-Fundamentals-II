#include <stdlib.h>
#include <stdio.h>

#include "ticker.h"
#include "bitstamp.h"
#include "debug.h"
#include "watchers.h"
#include "unistd.h"
#include <sys/types.h>
#include <signal.h>
// #include "flags.h"

WATCHER *bitstamp_watcher_start(WATCHER_TYPE *type, char *args[]) {
    WATCHER *wp = (WATCHER *)malloc(sizeof(WATCHER));
	// debug("args[0]: %s", args[0]);
	// debug("args[1]: %s", args[1]);
	int fd[2], pid;
	//create pipe
	if(pipe(fd) == -1) {
		perror("pipe failed");
		return NULL;
	}

	//fork
	pid = fork();
	if(pid == -1) {
		perror("fork failed");
		return NULL;
	}

	//child process
	if(!pid) {
		// redirect standard input to the write end of the pipe
		//child write to
        if (dup2(fd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            return NULL;
        }

        // redirect standard output to the read end of the pipe
		//child read from
        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            return NULL;
        }

		// Close unused file descriptors
        close(fd[0]);
        close(fd[1]);

		//execute uwsc
		// debug("execvp arg1: %s", type->argv[0]);
		// debug("execvp arg2: %s", type->argv[1]);
		if(execvp(type->argv[0], type->argv) == -1) {
			perror("execvp failed");
			return NULL;
		}
	} else {
		//parent process
		int argsCount = 0;
		while(args[argsCount] != NULL) {
			argsCount++;
		}
		argsCount += 1;

		char **mallocArgs;
		if(!(mallocArgs = malloc(sizeof(char *) * (argsCount)))) {
			perror("malloc failed");
			return NULL;
		}
		// debug("argsCount: %d", argsCount);
		for(int i = 0; i < argsCount; i++) {
			if(args[i]) {
				size_t argLen = strlen(args[i]);
				mallocArgs[i] = malloc(argLen + 1);
				strcpy(mallocArgs[i], args[i]);
			} else {
				mallocArgs[i] = NULL;
			}
		}
		// debug("mallocArgs[0]: %s", mallocArgs[0]);
		// debug("mallocArgs[1]: %s", mallocArgs[1]);

		*wp = (WATCHER){.wType = type, .pid = pid, .fd1 = fd[0], .fd2 = fd[1], .args = mallocArgs, .terminating = 0};
		// debug("wp->args[0]: %s", wp->args[0]);
	}

	return wp;
}

int bitstamp_watcher_stop(WATCHER *wp) {
	// pid_t pid = wp->pid;
	wp->terminating = 1;
	// debug("hi");
	// send the SIGTERM signal to the child process
	if (kill(wp->pid, SIGTERM) == -1) {
		perror("kill failed");
		// handle error
		return -1;
	}
	// debug("bitstamp_watcher_stop");

	// // wait for the child process to terminate
	// if (waitpid(pid, NULL, 0) == -1) {
	// 	perror("waitpid failed");
	// 	// handle error
	// 	return -1;
	// }

	// // free the memory allocated for the arguments
	// free(wp->args);
	// free(wp);
	// if(sigChildTermFlag) {
		
	// }
	return 0;

}

int bitstamp_watcher_send(WATCHER *wp, void *arg) {
	return write(wp->fd2, arg, strlen(arg));
}

int bitstamp_watcher_recv(WATCHER *wp, char *txt) {
    // TO BE IMPLEMENTED
    abort();
}

int bitstamp_watcher_trace(WATCHER *wp, int enable) {
    // TO BE IMPLEMENTED
    abort();
}
