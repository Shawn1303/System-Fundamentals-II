#include <stdlib.h>
#include <stdio.h>

#include "ticker.h"
#include "bitstamp.h"
#include "debug.h"
#include "watchers.h"
#include "unistd.h"

WATCHER *bitstamp_watcher_start(WATCHER_TYPE *type, char *args[]) {
    WATCHER *wp = (WATCHER *)malloc(sizeof(WATCHER));

	// int fd[2], pid;
	// //create pipe
	// if(pipe(fd) == -1) {
	// 	perror("pipe failed");
	// 	return NULL;
	// }

	// //fork
	// pid = fork();
	// if(pid == -1) {
	// 	perror("fork failed");
	// 	return NULL;
	// }

	// //child process
	// if(!pid) {
	// 	// redirect standard input to the write end of the pipe
    //     if (dup2(fd[0], STDIN_FILENO) == -1) {
    //         perror("dup2");
    //         return NULL;
    //     }

    //     // redirect standard output to the read end of the pipe
    //     if (dup2(fd[1], STDOUT_FILENO) == -1) {
    //         perror("dup2");
    //         return NULL;
    //     }

	// 	// Close unused file descriptors
    //     close(fd[0]);
    //     close(fd[1]);

	// 	//execute uwsc
	// 	execvp(type->argv[0], type->argv[1]);
	// } else {

	// }

	// *wp = (WATCHER){.wType = type, .pid = , .fd1 = , .fd2 = , .args = args};
	return wp;
}

int bitstamp_watcher_stop(WATCHER *wp) {
    // TO BE IMPLEMENTED
    abort();
}

int bitstamp_watcher_send(WATCHER *wp, void *arg) {
    // TO BE IMPLEMENTED
    abort();
}

int bitstamp_watcher_recv(WATCHER *wp, char *txt) {
    // TO BE IMPLEMENTED
    abort();
}

int bitstamp_watcher_trace(WATCHER *wp, int enable) {
    // TO BE IMPLEMENTED
    abort();
}
