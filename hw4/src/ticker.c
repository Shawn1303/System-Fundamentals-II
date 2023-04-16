#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "debug.h"
#include "watchers.h"
#include <sys/wait.h>
#include <time.h>
// #include "flags.h"

#include "ticker.h"

//quit flag
int quit;
//error flag
int err;
//keeps track of the max number of watchers I need to check
// int maxWatcherIndex;
//watcher table pointer
WATCHER **watcher_table;
//CLI
WATCHER *CLI;

//track of number of children allocated
int childrenAllocated = 0;
//track of number of children freed
int childrenFreed = 0;

volatile sig_atomic_t userInput = 0;
volatile sig_atomic_t processInput = 0;


void sigIntHandler(int sig) {
	debug("sigInt terminated\n");
	//figure out what to close and shit
	//don't exit
	exit(0);
	quit = 1;

}

void sigChildHandler(int sig) {
	// debug("sigChild terminated\n");
	int status, pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
		// debug("hi");
		// sigChildTermFlag = 1;
        // debug("Child process %d terminated\n", pid);

		//find the watcher that terminated
		int terminated = 1;
		// debug("CLI: %p", CLI);
		while(terminated < CLI->watcher_table_size) {
			// debug("wPid: %d" , CLI->watcher_table[terminated]->pid);
			// debug("pid: %d", pid);
			if(CLI->watcher_table[terminated]) {
				if(CLI->watcher_table[terminated]->pid == pid) {
					// debug("freeing");
					int argsIndex = 0;
					while(CLI->watcher_table[terminated]->args[argsIndex]) {
						// debug("args: %s", CLI->watcher_table[terminated]->args[argsIndex]);
						free(CLI->watcher_table[terminated]->args[argsIndex]);
						argsIndex++;
					}
					close(CLI->watcher_table[terminated]->fd1);
					close(CLI->watcher_table[terminated]->fd2);
					free(CLI->watcher_table[terminated]->args);
					free(CLI->watcher_table[terminated]);
					CLI->watcher_table[terminated] = NULL;
					// CLI->watcher_table_size--;
					if(quit) {
						childrenFreed += 1;
						// debug("childrenF %d", childrenFreed);
					}
					break;
				}
			}
			terminated++;
		}
    }
	// debug("hi");
	// debug("watcher2: %d", CLI->watcher_table[2]->pid);

	if(pid == -1 && errno != ECHILD) {
		perror("waitpid failed");
		err = 1;
		// exit(1);
	}
	processInput = 0;
	// debug("quit, %d", quit);
	// if(quit) {
	// 	childrenFreed += 1;
	// debug("childrenF %d", childrenFreed);
	// }
	//don't exit
	// exit(0);
}

//becomes 1 if userinput is avaliable
// volatile sig_atomic_t userInput = 0;
// volatile sig_atomic_t processInput = 0;

// void sigIOHandler(int sig, siginfo_t *siginfo, void *context) {
// 	//execute first
// 	//gets stdin file descriptor
//     int fd = siginfo->si_fd;

// 	// if user input is avaliable on stdin
// 	if(fd == STDIN_FILENO) {
// 		// user input avaliable
// 		debug("fd: %d", fd);
// 		userInput = 1;
// 	} else {
// 		// debug("fd: %d", fd);
// 		// processInput = fd;
// 		if(readFromProcess(fd) == -1) {
// 			quit = 1;
// 			err = 1;
// 		}
// 	}
// 	// sigIO = 1;
// }

// int outputTracing(WATCHER *wp, char *traceMsg) {
// 	debug("traceMsg: %s", traceMsg);
// 	debug("wp->tracing: %d", wp->tracing);
// 	if(wp->tracing) {
// 		// debug("traceMsg: %s", traceMsg);
// 		// debug("wp->tracing: %d", wp->tracing);
// 		debug("wp->fd2: %d", wp->fd2);
// 		struct timespec ts;
// 		if(clock_gettime(CLOCK_REALTIME, &ts) == -1) {
// 			perror("clock_gettime failed");
// 			return -1;
// 		}
// 		// int microsecs = ts.tv_nsec / 1000;
// 		if(fprintf(stderr, "[%ld.%06ld][%s][%d][%d]: [%s]", 
// 		ts.tv_sec, ts.tv_nsec / 1000, wp->wType->name, wp->fd1, wp->fd2, traceMsg)) {
// 			perror("fprintf failed");
// 			return -1;
// 		}
// 	}
// 	return 0;
// }

// void readFromInput(sigset_t *mask) {
int readFromInput() {
	//reads input
	static char *buf = NULL;
	static size_t buf_size = 0;
	static FILE *memstream = NULL;

	//open memstream, called once
	if(!(memstream)) {
		if(!(memstream = open_memstream(&buf, &buf_size))) {
			perror("open_memstream failed");
			// exit(1);
			return -1;
		}
	}

	//read from stdin until no more input
	int n;
	char input[100];
	while((n = read(STDIN_FILENO, input, sizeof(input))) > 0) {
		// debug("n: %d", n);
		// debug("input: %s", input);
		//write to memstream
		if ((fwrite(input, sizeof(char), n, memstream)) < 0) {
			perror("fwrite failed");
			// exit(1);
			// return -1;
			err = 1;
			break;
		}
	}
	// debug("n after read: %d", n);

	// FILE *stream;
	// if((stream = fmemopen(buf, buf_size, "r")) == NULL) {
	// 	perror("fmemopen failed");
	// 	exit(1);
	// }
	//if error
	if(n < 0) { 
		if(errno == EWOULDBLOCK) {
			userInput = 0;
			// sigsuspend(&mask);
		} else {
			// debug("%d", errno);
			perror("read failed");
			// exit(1);
			// return -1;
			err = 1;
		}
	}

	//parse command line into args
	char *command = NULL;
	size_t command_size = 0;
	// debug("word: %s", fgets(command, sizeof(command), memstream));
	// debug("command: %s", command);
	//dynamically allocates memory for reading command lines
	while(getline(&command, &command_size, memstream) != -1) {
		// debug("hi");
		//set newline to null pointer
		// debug("command arg: %d", command[strlen(command)]);
		command[strlen(command) - 1] = '\0';



		// //get number of spaces
		// int spaceCount = 0;
		// for(int i = 0; i < strlen(command); i++) {
		// 	if(command[i] == ' ') {
		// 		spaceCount++;
		// 	}
		// }
		// char *commandArgs[spaceCount + 2];
		// //use tokens to parse command line
		// char *token = strtok(command, " ");
		// int argvIndex = 0;
		// while(token != NULL) {
		// 	commandArgs[argvIndex++] = token;
		// 	token = strtok(NULL, " ");
		// }
		// //last argv is null
		// commandArgs[argvIndex] = NULL;
		// // debug("command: %d", command[strlen(command)]);

		// //handle command
		// handleCommand(commandArgs);

		//recv return 1 if quit
		int recv = CLI->wType->recv(CLI, command);
		if( recv == 1) {
			quit = 1;
		} else if(recv == -1) {
			err = 1;
		}




		//next command if any
		// write(STDOUT_FILENO, "ticker> ", 8);
		if(CLI->wType->send(CLI, "ticker> ") < 0) {
			perror("send failed");
			// debug("hi");
			// exit(1);
			// free(command);
			// command = NULL;
			// command_size = 0;
			// return -1;
			err = 1;
		}

		//free array after use
		free(command);
		command = NULL;
		command_size = 0;

		//if command is quit, break
		if(quit || err) {
			break;
		}
	}
	//free array after use
	free(command);
	command = NULL;
	command_size = 0;

	//apparently returns 0 in nonblocking mode when EOF for file descriptor or something and I need to quit
	if(n == 0) {
		quit = 1;
	}

	//close memstream and free memory
	if((quit || err) && memstream) {
		// debug("quit: %d", quit);
		// debug("n: %d", n);
		// debug("memstream: %p", memstream);
		if(fclose(memstream) == EOF) {
			perror("fclose memstream failed");
			// exit(1);
			// return -1;
			err = 1;
		}
		free(buf);
		buf = NULL;
		memstream = NULL;
		buf_size = 0;
	}

	if(err) return -1;

	return 0;
}

int readFromProcess(int fd) {
	static char *buf = NULL;
	static size_t buf_size = 0;
	static FILE *memstream = NULL;
	WATCHER *sender = NULL;

	for(int i = 1; i < CLI->watcher_table_size; i++) {
		if(CLI->watcher_table[i]) {
			if(CLI->watcher_table[i]->fd1 == fd) {
				sender = CLI->watcher_table[i];
				// debug("sender: %d", sender->fd1);
				// debug("fd: %d", fd);
				break;
			}
		}
	}

	//open memstream, called once
	if(!(memstream)) {
		if(!(memstream = open_memstream(&buf, &buf_size))) {
			perror("open_memstream failed");
			// exit(1);
			return -1;
		}
	}

	//read from stdin until no more input
	int m;
	char received[100];
	while((m = read(fd, received, sizeof(received))) > 0) {
		// debug("n: %d", n);
		// debug("input: %s", input);
		//write to memstream
		if ((fwrite(received, sizeof(char), m, memstream)) < 0) {
			perror("fwrite failed");
			// exit(1);
			// return -1;
			err = 1;
			break;
		}
		// debug("input: %s", msg);
	}

	if(m < 0) { 
		if(errno == EWOULDBLOCK) {
			processInput = 0;
			// sigsuspend(&mask);
		} else {
			// debug("%d", errno);
			perror("read failed");
			// exit(1);
			// return -1;
			err = 1;
		}
	}

	//parse msg
	char *msg = NULL;
	size_t msg_size = 0;
	// debug("word: %s", fgets(command, sizeof(command), memstream));
	// debug("command: %s", command);
	//dynamically allocates memory for reading command lines
	while(getline(&msg, &msg_size, memstream) != -1) {
		// debug("hi");
		//set newline to null pointer
		// debug("msg arg: %d", msg[strlen(msg) - 1]);
		// debug("msg: %s", msg);
		msg[strlen(msg) - 1] = '\0';


		int recv = sender->wType->recv(sender, msg);
		// debug("recv: %d", recv);
		if( recv == 1) {
			quit = 1;
		} else if(recv == -1) {
			err = 1;
		}


		// if(CLI->wType->send(CLI, "ticker> ") < 0) {
		// 	perror("send failed");
		// }

		//free array after use
		free(msg);
		msg = NULL;
		msg_size = 0;

		//if command is quit, break
		if(quit || err) {
			break;
		}
	}
	//free array after use
	free(msg);
	msg = NULL;
	msg_size = 0;

	// if((quit || err) && memstream) {
		// debug("quit: %d", quit);
		// debug("n: %d", n);
		// debug("memstream: %p", memstream);
	if(fclose(memstream) == EOF) {
		perror("fclose memstream failed");
		// exit(1);
		// return -1;
		err = 1;
	}
	free(buf);
	buf = NULL;
	memstream = NULL;
	buf_size = 0;
	// }

	if(err) return -1;
	
	return 0;
}

void sigIOHandler(int sig, siginfo_t *siginfo, void *context) {
	//execute first
	//gets stdin file descriptor
    int fd = siginfo->si_fd;

	// if user input is avaliable on stdin
	if(fd == STDIN_FILENO) {
		// user input avaliable
		// debug("fd: %d", fd);
		userInput = 1;
	} else {
		// debug("fd: %d", fd);
		// processInput = fd;
		// if(readFromProcess(fd) == -1) {
		// 	quit = 1;
		// 	err = 1;
		// }
		processInput = fd;
	}
	// sigIO = 1;
}

int ticker(void) {
	struct sigaction action1, action2, action3;
	//initialize sigactions
	if(memset(&action1, 0, sizeof(action1)) == NULL) {
		perror("memset failed");
		return 1;
	}
	action1.sa_handler = sigIntHandler;
	if(sigemptyset(&action1.sa_mask)) {
		perror("sigemptyset");
		return 1;
	}
	if(sigaction(SIGINT, &action1, NULL) < 0) {
		perror("sigaction SIGINT");
		return 1;
	}

	if(memset(&action2, 0, sizeof(action2)) == NULL) {
		perror("memset failed");
		return 1;
	}
	action2.sa_handler = sigChildHandler;
	if(sigemptyset(&action2.sa_mask)) {
		perror("sigemptyset");
		return 1;
	}
	if(sigaction(SIGCHLD, &action2, NULL) < 0) {
		perror("sigaction SIGCHLD");
		return 1;
	}

	if(memset(&action3, 0, sizeof(action3)) == NULL) {
		perror("memset failed");
		return 1;
	}
	action3.sa_sigaction = sigIOHandler;
	action3.sa_flags = SA_SIGINFO;
	if(sigemptyset(&action3.sa_mask)) {
		perror("sigemptyset");
		return 1;
	}
	if(sigaction(SIGIO, &action3, NULL) < 0) {
		perror("sigaction SIGIO");
		return 1;
	}
//test handlers
	// kill(getpid(), SIGCHLD);	
	// kill(getpid(), SIGIO);



// To enable asynchronous I/O notification on a file descriptor

// (1) Asynchronous I/O notification needs to be set on the file descriptor,
// using the fcntl() system call with command F_SETFL and argument
// O_ASYNC
	if(fcntl(STDIN_FILENO, F_SETFL, O_ASYNC | O_NONBLOCK) < 0) {
		perror("fcntl F_SETFL");
		return 1;
	}

// The fcntl() system call has to be used on that file descriptor with
// command F_SETOWN to identity the process to which I/O notifications for
// that file descriptor should be sent;
	if(fcntl(STDIN_FILENO, F_SETOWN, getpid()) < 0) {
		perror("fcntl F_SETOWN");
		return 1;
	}

// The fcntl() system call has to be used
// on that file descriptor with command F_SETSIG to designate
// SIGIO as the signal to be sent as the asynchronous I/O notification
	if(fcntl(STDIN_FILENO, F_SETSIG, SIGIO) < 0) {
		perror("fcntl F_SETSIG");
		return 1;
	}



	// if (fcntl(STDOUT_FILENO, F_SETFL, O_ASYNC | O_NONBLOCK) == -1) {
    //     perror("Error: cannot set non-blocking mode for stdout");
    //     return 1;
    // }
    // if (fcntl(STDOUT_FILENO, F_SETOWN, getpid()) == -1) {
    //     perror("Error: cannot set process owner for stdout");
    //     return 1;
    // }
    // if (fcntl(STDOUT_FILENO, F_SETSIG, SIGIO) == -1) {
    //     perror("Error: cannot set signal for stdout");
    //     return 1;
    // }




	//start CLI and table
	//watcher_types[CLI_WATCHER_TYPE].argv
	if(!(CLI = watcher_types[CLI_WATCHER_TYPE].start(&watcher_types[CLI_WATCHER_TYPE], watcher_types[CLI_WATCHER_TYPE].argv))) {
		perror("CLI start failed");
		return 1;
	}
	int watcher_table_size = 10;
	if(!(watcher_table = malloc(sizeof(WATCHER *) * watcher_table_size))) {
		perror("malloc watcher_table failed");
		free(CLI);
		return 1;
	}
	CLI->watcher_table = watcher_table;
	CLI->watcher_table_size = watcher_table_size;
	// debug("%p", &watcher_table_size);
	// debug("%p", &CLI->watcher_table_size);
	watcher_table[0] = CLI;
	//used to keep track of max index in watcher_table
	// maxWatcherIndex = 0;
	// CLI->maxWatcherIndex = 0;
	watcher_table[1] = NULL;
	for(int i = 2; i < watcher_table_size; i++) {
		watcher_table[i] = NULL;
	}

	//used for sigsuspend
	sigset_t mask;
	if(sigemptyset(&mask) == -1) {
		perror("sigemptyset failed");
		// return 1;
		quit = 1;
		err = 1;
	}

	//first command prompt
	// write(STDOUT_FILENO, "ticker> ", 8);
	if(CLI->wType->send(CLI, "ticker> ") < 0) {
		perror("send failed");
		quit = 1;
		err = 1;
	}
	// printf("ticker> ");
	//read first to check for any commands
	if(readFromInput() == -1) {
		quit = 1;
		err = 1;
	}

	while(!quit && !err) {
		// debug("hi");
		if(!userInput && !processInput) {
			sigsuspend(&mask);
		}
		// debug("userInput: %d", userInput);
		if(userInput) {
			// sigIO = 0;
			if(readFromInput() == -1) {
				quit = 1;
				err = 1;
			}
		}
		// debug("userInput: %d", userInput);
		// debug("processInput: %d", processInput);
		// debug("err: %d", err);
		// debug("quit: %d", quit);
		if(quit || err) {
			// debug("hi");
			sigset_t sigset;
			if (sigemptyset(&sigset)) {
				perror("sigemptyset");
				// return 1;
				err = 1;
			}
			if (sigaddset(&sigset, SIGIO)) {
				perror("sigaddset");
				// return 1;
				err = 1;
			}
			if (sigprocmask(SIG_BLOCK, &sigset, NULL)) {
				perror("sigprocmask");
				// return 1;
				err = 1;
			}
		}
		if(processInput) {
			// debug("processInput");
			if(readFromProcess(processInput) == -1) {
				quit = 1;
				err = 1;
			}
		}
		// if(watcher_table[1]) {
		// 	debug("args: %s", watcher_table[1]->args[0]);
		// }
		// debug("%s", watcher_types[0].name);
		// debug("%s", watcher_types[1].name);
		// debug("%s", watcher_types[2].name);

		//execute second
		// write(STDOUT_FILENO, "ticker> ", 8);
		// printf("ticker> ");

		//reads input
		// char input[100];
		// ssize_t n;
		// while((n = read(fd, input, sizeof(input))) > 0) {
		// 	debug("input size: %zu", sizeof(input));
		// 	debug( "read %zu bytes", n);
		// 	debug( "%s", input);
		// 	fwrite(input, sizeof(char), n, memstream);
		// 	nRead += n;
		// 	for(int i = 0; i < n; i++) {
		// 		if(input[i] == '\n') {
		// 			// nRead = n - i - 1;
		// 		}
		// 	}
		// }
	}


	// if (quit) {
    // sigset_t sigset;
    // if (sigemptyset(&sigset)) {
    //     perror("sigemptyset");
    //     // return 1;
	// 	err = 1;
    // }
    // if (sigaddset(&sigset, SIGIO)) {
    //     perror("sigaddset");
    //     // return 1;
	// 	err = 1;
    // }
    // if (sigprocmask(SIG_BLOCK, &sigset, NULL)) {
    //     perror("sigprocmask");
    //     // return 1;
	// 	err = 1;
    // }


	//quit happens
	int closeIndex = 1;
	// debug("%p", &watcher_table_size);
	// debug("%p", &CLI->watcher_table_size);
	//malloc the size ---------------------------------------------------------------------------------
	while(closeIndex < CLI->watcher_table_size) {
		if(watcher_table[closeIndex]) {
			// debug("hi");
			if(watcher_table[closeIndex]->wType->stop(watcher_table[closeIndex]) < 0) {
				perror("watcher stop failed");
				return 1;
			} else {
				childrenAllocated++;
			}
		}
		closeIndex++;
	}
	// debug("childrenAllocated: %d", childrenAllocated);
	// debug("childrenFreed: %d", childrenFreed);
	while(childrenFreed != childrenAllocated);
	// debug("hi");
	//stop CLI
	if(CLI->wType->stop(CLI) < 0) {
		perror("CLI stop failed");
		return 1;
	} else {
		CLI = NULL;
		watcher_table = NULL;
	}
	if(err)
		return 1;
	return 0;
}
