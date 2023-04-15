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
	int fd1[2], fd2[2], pid;
	//C => P, P => C
	//create pipe
	if(pipe(fd1) == -1) {
		perror("pipe1 failed");
		return NULL;
	}
	if(pipe(fd2) == -1) {
		perror("pipe2 failed");
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
		//child don't use pipe1 to read, only write
		close(fd1[0]);
		//child don't use pipe2 to write, only read
		close(fd2[1]);
		// redirect standard input to the read end of the pipe
		//child write to
        if (dup2(fd2[0], STDIN_FILENO) == -1) {
            perror("dup2");
            return NULL;
        }

        // redirect standard output to the write end of the pipe
		//child read from
        if (dup2(fd1[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            return NULL;
        }

		// Close unused file descriptors
        close(fd2[0]);
        close(fd1[1]);
		//-------------------------------------
		// char fromP[1024];
		// int a = read(STDIN_FILENO, fromP, sizeof(fromP));
		// debug("a: %d", a);

		// if (a < 0) {
		// 	perror("read");
		// 	return NULL;
		// }

		// debug("fromP: %s\n", fromP);
		// debug("strlen(fromP): %zu", strlen(fromP));

		// write(STDOUT_FILENO, "hello\n", 5);

		//execute uwsc
		// debug("execvp arg1: %s", type->argv[0]);
		// debug("execvp arg2: %s", type->argv[1]);
		// type->argv[1] = "hello";
		if(execvp(type->argv[0], type->argv) == -1) {
			perror("execvp failed");
			return NULL;
		}
	} else {
		//parent don't use pipe1 to write, only read
		close(fd1[1]);
		//parent don't use pipe2 to read, only write
		close(fd2[0]);
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

		*wp = (WATCHER){.wType = type, .pid = pid, .fd1 = fd1[0], .fd2 = fd2[1], .args = mallocArgs, .terminating = 0};
		// debug("wp->args[0]: %s", wp->args[0]);
		int jsonComSize = snprintf(NULL, 0, "{ \"event\": \"bts:subscribe\", \"data\": { \"channel\": \"%s\" } }\n", args[0]);
		char jsonCom[jsonComSize + 1];
		sprintf(jsonCom, "{ \"event\": \"bts:subscribe\", \"data\": { \"channel\": \"%s\" } }\n", args[0]);
		jsonCom[jsonComSize] = '\0';
		// debug("jsonCom[jsonComSize]: %c", jsonCom[jsonComSize - 1]);
		// debug("jsonCom: %s", jsonCom);
		// debug("jsonSize: %d", jsonComSize);
		// debug("strlen(jsonCom): %zu", strlen(jsonCom));
		// debug("args[0][strlen(args[0]) - 1]: %d", args[0][strlen(args[0]) - 1]);

		// read the acknowledgement from child process
		// char ack[1024];
		// int nbytes = read(wp->fd1, ack, sizeof(ack));

		// if (nbytes < 0) {
		// 	perror("read");
		// 	return NULL;
		// }
		// // debug("nbytes: %d", nbytes);
		// // debug("strlen(ack): %zu", strlen(ack));
		// // debug("ack[nbytes - 1]: %c", ack[nbytes - 2]);
		// // debug("ack[nbytes - 1]: %d", ack[nbytes - 2]);
		// ack[nbytes] = '\0';

		// debug("%s\n", ack);
		// if(strcmp(ack, "Acknowledgement received: Websocket connected, you can send text messages of maximum 256 characters.\nTo exit uwsc, type !q<enter>\n>")) {
		// 	// debug("hi");
		// 	write(fd2[1], jsonCom, strlen(jsonCom) + 1);
		// }

		// nbytes = read(wp->fd1, ack, sizeof(ack));

		// if (nbytes < 0) {
		// 	perror("read");
		// 	return NULL;
		// }

		// ack[nbytes] = '\0';
		// debug("%s\n", ack);
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
