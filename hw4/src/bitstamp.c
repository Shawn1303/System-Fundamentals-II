#include <stdlib.h>
#include <stdio.h>

#include "ticker.h"
#include "bitstamp.h"
#include "debug.h"
#include "watchers.h"
#include "unistd.h"
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <argo.h>
#include <store.h>
#include "helperFun.h"
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

		if(fcntl(fd1[0], F_SETFL, O_ASYNC | O_NONBLOCK) < 0) {
			perror("fcntl F_SETFL");
			return NULL;
		}

	// The fcntl() system call has to be used on that file descriptor with
	// command F_SETOWN to identity the process to which I/O notifications for
	// that file descriptor should be sent;
		if(fcntl(fd1[0], F_SETOWN, getpid()) < 0) {
			perror("fcntl F_SETOWN");
			return NULL;
		}

	// The fcntl() system call has to be used
	// on that file descriptor with command F_SETSIG to designate
	// SIGIO as the signal to be sent as the asynchronous I/O notification
		if(fcntl(fd1[0], F_SETSIG, SIGIO) < 0) {
			perror("fcntl F_SETSIG");
			return NULL;
		}


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

		*wp = (WATCHER){.wType = type, .pid = pid, .fd1 = fd1[0], .fd2 = fd2[1], .args = mallocArgs, .terminating = 0, .tracing = 0, .serial = 0};
		// debug("wp->args[0]: %s", wp->args[0]);
		// int jsonComSize = snprintf(NULL, 0, "{ \"event\": \"bts:subscribe\", \"data\": { \"channel\": \"%s\" } }\n", args[0]);
		// char jsonCom[jsonComSize + 1];
		// sprintf(jsonCom, "{ \"event\": \"bts:subscribe\", \"data\": { \"channel\": \"%s\" } }\n", args[0]);
		// jsonCom[jsonComSize] = '\0';
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
	if(arg) {
		return write(wp->fd2, arg, strlen(arg) + 1);
	}
	return 0;
}

int bitstamp_watcher_recv(WATCHER *wp, char *txt) {
	wp->serial++;
	// debug("hi");
	// int error = 0;
	char *wrapper;
	int offset = 0;
	if(!(wrapper = malloc(strlen(txt) + 1))) {
		perror("malloc failed");
		return -1;
	}
	strcpy(wrapper, txt);
	// debug("wrapper: %s", wrapper);
	// debug("wrapper[strlen(txt)]: %d", wrapper[strlen(txt) - 1]);
	// debug("wrapper[strlen(txt)]: %c", wrapper[strlen(txt) - 1]);
	wrapper[strlen(txt) - 1] = '\0';
	while((wrapper[0] < 65 || wrapper[0] > 90) && wrapper[0] != '\0' && wrapper[0] != '>') {
		// debug("wrapper[0]: %c", wrapper[0]);
		// debug("wrapper[0]: %d", wrapper[0]);
		wrapper += 1;
		offset += 1;
	}
	if(outputTracing(wp, wrapper) == -1) {
		return -1;
	}
	int jsonSize = 0;
	char *temp = NULL;
	// wrapper[strlen(txt) - 1] = '\0';
	for(int i = 0; i < strlen(wrapper); i++) {
		if(wrapper[i] == '\'') {
			// debug("i: %d", i);
			// debug("wrapper[i]: %c", wrapper[i - 1]);
			// debug("wrapper[i]: %d", wrapper[i - 1]);
			temp = wrapper + i + 1;
			jsonSize = strlen(wrapper + i + 1);
			wrapper[i] = '\0';
			break;
		}
	}
	char json[jsonSize + 1];
	// if(!(json = malloc(jsonSize + 1))) {
	// 	perror("malloc failed");
	// 	return -1;
	// }
	if(temp) {
		strcpy(json, temp);
	}
	json[jsonSize] = '\0';
	// debug("json: %s", json);
	// debug("wrapper: %s", wrapper);

	FILE *fmemstream;

	if(!(fmemstream = fmemopen(json, strlen(json), "r"))) {
		// debug("hi");
		perror("fmemopen failed");
		free(wrapper - offset);
		return -1;
		// error = 1;
	}
	// debug("hi");

	ARGO_VALUE *jsonValue = NULL;
	//check websocket connection
	// debug("%s", txt);
	// debug("wrapper: %s", wrapper);
	// debug("same? %d", (strcmp(wrapper, "Server message: ")));
	// debug("strlen(wrapper): %zu", strlen(wrapper));
	// debug("wrapper[strlen(wrapper) - 1]: %d", wrapper[strlen(wrapper) - strlen(wrapper)]);
	// debug("wrapper[strlen(wrapper) - 1]: %c", wrapper[strlen(wrapper) - strlen(wrapper)]);
	if(!strcmp(txt, "Websocket connected, you can send text messages of maximum 256 characters.")) {
		// debug("hi");
		// debug("txt: %s", txt);
		int jsonComSize; 
		// debug("wp->args[0]: %s", wp->args[0]);
		if((jsonComSize = snprintf(NULL, 0, "{ \"event\": \"bts:subscribe\", \"data\": { \"channel\": \"%s\" } }\n", wp->args[0])) < 0) {
			perror("snprintf failed");
			free(wrapper - offset);
			return -1;
		}
		char jsonCom[jsonComSize + 1];
		if((sprintf(jsonCom, "{ \"event\": \"bts:subscribe\", \"data\": { \"channel\": \"%s\" } }\n", wp->args[0])) < 0) {
			perror("sprintf failed");
			free(wrapper - offset);
			return -1;
		}
		jsonCom[jsonComSize] = '\0';
		// debug("jsonCom: %s", jsonCom);
		wp->wType->send(wp, jsonCom);
	} else if(!(strcmp(wrapper, "Send "))) {
		//check if same channel
		// debug("hi");
		//send json
		if(!(jsonValue = argo_read_value(fmemstream))) {
			perror("argo_read_value failed");
			// argo_free_value(jsonValue);
			free(wrapper - offset);
			if(fclose(fmemstream) == EOF) {
				perror("fclose failed");
				return -1;
			}
			return -1;
		}
		ARGO_VALUE *sendData;
		if(!(sendData = argo_value_get_member(jsonValue, "data"))) {
			perror("argo_value_get_member failed");
			if(fclose(fmemstream) == EOF) {
				perror("fclose failed");
				return -1;
			}
			argo_free_value(jsonValue);
			free(wrapper - offset);
			return -1;
		}
		ARGO_VALUE *channelName;
		if(!(channelName = argo_value_get_member(sendData, "channel"))) {
			perror("argo_value_get_member failed");
			if(fclose(fmemstream) == EOF) {
				perror("fclose failed");
				return -1;
			}
			//argo_free_value(sendData);
			argo_free_value(jsonValue);
			free(wrapper - offset);
			return -1;
		}
		char *channelNameStr;
		if(!(channelNameStr = argo_value_get_chars(channelName))) {
			perror("argo_value_to_string failed");
			//argo_free_value(channelName);
			if(fclose(fmemstream) == EOF) {
				perror("fclose failed");
				return -1;
			}
			//argo_free_value(sendData);
			argo_free_value(jsonValue);
			free(wrapper - offset);
			return -1;
		}
		// debug("channelNameStr: %s", channelNameStr);
		// debug("wp->args[0]: %s", wp->args[0]);
		// debug("same? %d", strcmp(channelNameStr, wp->args[0]));
		// debug("strlen(channelNameStr): %zu", strlen(channelNameStr));
		// debug("strlen(wp->args[0]): %zu", strlen(wp->args[0]));
		if(strcmp(channelNameStr, wp->args[0])) {
			perror("Different channel");
			free(channelNameStr);
			//argo_free_value(channelName);
			if(fclose(fmemstream) == EOF) {
				perror("fclose failed");
				return -1;
			}
			//argo_free_value(sendData);
			argo_free_value(jsonValue);
			free(wrapper - offset);
			return -1;
		}
		//argo_free_value(sendData);
		free(channelNameStr);
		//argo_free_value(channelName);
	} else if(!(strcmp(wrapper, "Server message: "))) {
		// debug("hi");
		//server message json
		if(!(jsonValue = argo_read_value(fmemstream))) {
			perror("argo_read_value failed");
			// argo_free_value(jsonValue);
			free(wrapper - offset);
			if(fclose(fmemstream) == EOF) {
				perror("fclose failed");
				return -1;
			}
			return -1;
		}
		ARGO_VALUE *event;
		if(!(event = argo_value_get_member(jsonValue, "event"))) {
			perror("argo_value_get_member failed");
			if(fclose(fmemstream) == EOF) {
				perror("fclose failed");
				return -1;
			}
			argo_free_value(jsonValue);
			free(wrapper - offset);
			return -1;
		}
		// argo_free_value(jsonValue);
		char *eventName;
		if(!(eventName = argo_value_get_chars(event))) {
			perror("argo_value_to_string failed");
			//argo_free_value(event);
			if(fclose(fmemstream) == EOF) {
				perror("fclose failed");
				return -1;
			}
			argo_free_value(jsonValue);
			free(wrapper - offset);
			return -1;
		}
		// debug("hiiiiiiiii");
		if(!(strcmp(eventName, "trade"))) {
			// debug("trade");
			ARGO_VALUE *data;
			if(!(data = argo_value_get_member(jsonValue, "data"))) {
				perror("argo_value_get_member failed");
				free(eventName);
				//argo_free_value(event);
				if(fclose(fmemstream) == EOF) {
					perror("fclose failed");
					return -1;
				}
				argo_free_value(jsonValue);
				free(wrapper - offset);
				return -1;
			}
			// ARGO_VALUE *price;
			// if(!(price = argo_value_get_member(data, "price"))) {
			// 	perror("argo_value_get_member failed");
			// 	free(eventName);
			// 	//argo_free_value(event);
			// 	//argo_free_value(data);
			// 	if(fclose(fmemstream) == EOF) {
			// 		perror("fclose failed");
			// 		return -1;
			// 	}
			// 	argo_free_value(jsonValue);
			// 	free(wrapper - offset);
			// 	return -1;
			// }
			// ARGO_VALUE *amount;
			// if(!(amount = argo_value_get_member(data, "amount"))) {
			// 	perror("argo_value_get_member failed");
			// 	free(eventName);
			// 	//argo_free_value(event);
			// 	//argo_free_value(data);
			// 	//argo_free_value(price);
			// 	if(fclose(fmemstream) == EOF) {
			// 		perror("fclose failed");
			// 		return -1;
			// 	}
			// 	argo_free_value(jsonValue);
			// 	free(wrapper - offset);
			// 	return -1;
			// }
			long priceLong;
			if(argo_value_get_long(argo_value_get_member(data, "price"), &priceLong) == -1) {
				perror("argo_value_to_double failed");
				free(eventName);
				//argo_free_value(event);
				//argo_free_value(data);
				//argo_free_value(price);
				//argo_free_value(amount);
				if(fclose(fmemstream) == EOF) {
					perror("fclose failed");
					return -1;
				}
				argo_free_value(jsonValue);
				free(wrapper - offset);
				return -1;
			}
			// debug("price: %ld", priceLong);
			double amountDouble;
			if(argo_value_get_double(argo_value_get_member(data, "amount"), &amountDouble)) {
				perror("argo_value_to_long failed");
				free(eventName);
				//argo_free_value(event);
				//argo_free_value(data);
				//argo_free_value(price);
				//argo_free_value(amount);
				// free(priceStr);
				if(fclose(fmemstream) == EOF) {
					perror("fclose failed");
					return -1;
				}
				argo_free_value(jsonValue);
				free(wrapper - offset);
				return -1;
			}
			// debug("amount: %f", amountDouble);







			//making price key
			int priceKeyLen;
			if((priceKeyLen = snprintf(NULL, 0, "%s:%s:%s", wp->wType->name, wp->args[0], "price")) < 0) {
				perror("snprintf failed");
				free(eventName);
				//argo_free_value(event);
				if(fclose(fmemstream) == EOF) {
					perror("fclose failed");
					return -1;
				}
				argo_free_value(jsonValue);
				free(wrapper - offset);
				return -1;
			}
			char *priceKey;
			if((priceKey = malloc(priceKeyLen + 1)) == NULL) {
				perror("malloc failed");
				free(eventName);
				//argo_free_value(event);
				if(fclose(fmemstream) == EOF) {
					perror("fclose failed");
					return -1;
				}
				argo_free_value(jsonValue);
				free(wrapper - offset);
				return -1;
			}
			if(sprintf(priceKey, "%s:%s:%s", wp->wType->name, wp->args[0], "price") < 0) {
				perror("sprintf failed");
				free(eventName);
				//argo_free_value(event);
				if(fclose(fmemstream) == EOF) {
					perror("fclose failed");
					return -1;
				}
				argo_free_value(jsonValue);
				free(wrapper - offset);
				return -1;
			}
			//volume key
			int volumeLen;
			if((volumeLen = snprintf(NULL, 0, "%s:%s:%s", wp->wType->name, wp->args[0], "volume")) < 0) {
				perror("snprintf failed");
				free(eventName);
				//argo_free_value(event);
				if(fclose(fmemstream) == EOF) {
					perror("fclose failed");
					return -1;
				}
				argo_free_value(jsonValue);
				free(wrapper - offset);
				return -1;
			}
			char *volumeKey;
			if((volumeKey = malloc(volumeLen + 1)) == NULL) {
				perror("malloc failed");
				free(eventName);
				//argo_free_value(event);
				if(fclose(fmemstream) == EOF) {
					perror("fclose failed");
					return -1;
				}
				argo_free_value(jsonValue);
				free(wrapper - offset);
				return -1;
			}
			if(sprintf(volumeKey, "%s:%s:%s", wp->wType->name, wp->args[0], "volume") < 0) {
				perror("sprintf failed");
				free(eventName);
				//argo_free_value(event);
				if(fclose(fmemstream) == EOF) {
					perror("fclose failed");
					return -1;
				}
				argo_free_value(jsonValue);
				free(wrapper - offset);
				return -1;
			}




			//free last price
			struct store_value *lastPriceValue;
			if((lastPriceValue = store_get(priceKey))) {
				store_free_value(lastPriceValue);
				store_put(priceKey, NULL);
			}
			//store price
			struct store_value *priceValue;
			if((priceValue = malloc(sizeof(struct store_value))) == NULL) {
				perror("malloc failed");
				free(eventName);
				//argo_free_value(event);
				if(fclose(fmemstream) == EOF) {
					perror("fclose failed");
					return -1;
				}
				argo_free_value(jsonValue);
				free(wrapper - offset);
				return -1;
			}
			priceValue->type = STORE_LONG_TYPE;
			priceValue->content.long_value = priceLong;
			if(store_put(priceKey, priceValue)) {
				perror("store_put failed");
				free(eventName);
				//argo_free_value(event);
				if(fclose(fmemstream) == EOF) {
					perror("fclose failed");
					return -1;
				}
				argo_free_value(jsonValue);
				free(wrapper - offset);
				return -1;
			}
			free(priceKey);



			//store volume
			struct store_value *oldVolume;
			if((oldVolume = store_get(volumeKey))) {
				double newVolume = amountDouble + oldVolume->content.double_value;
				// debug("newVolume: %f", newVolume);
				// int newVolumeLen;
				// if((newVolumeLen = snprintf(NULL, 0, "%ld", newVolume)) < 0) {
				// 	perror("snprintf failed");
				// 	free(eventName);
				// 	//argo_free_value(event);
				// 	if(fclose(fmemstream) == EOF) {
				// 		perror("fclose failed");
				// 		return -1;
				// 	}
				// 	argo_free_value(jsonValue);
				// 	free(wrapper - offset);
				// 	return -1;
				// }
				// char newVolumeStr[newVolumeLen + 1];
				// if(sprintf(newVolumeStr, "%ld", newVolume) < 0) {
				// 	perror("sprintf failed");
				// 	free(eventName);
				// 	//argo_free_value(event);
				// 	if(fclose(fmemstream) == EOF) {
				// 		perror("fclose failed");
				// 		return -1;
				// 	}
				// 	argo_free_value(jsonValue);
				// 	free(wrapper - offset);
				// 	return -1;
				// }
				struct store_value *newVolumeValue;
				if((newVolumeValue = malloc(sizeof(struct store_value))) == NULL) {
					perror("malloc failed");
					free(eventName);
					//argo_free_value(event);
					if(fclose(fmemstream) == EOF) {
						perror("fclose failed");
						return -1;
					}
					argo_free_value(jsonValue);
					free(wrapper - offset);
					return -1;
				}
				newVolumeValue->type = STORE_DOUBLE_TYPE;
				newVolumeValue->content.double_value = newVolume;
				if(store_put(volumeKey, newVolumeValue)) {
					perror("store_put failed");
					free(eventName);
					//argo_free_value(event);
					if(fclose(fmemstream) == EOF) {
						perror("fclose failed");
						return -1;
					}
					argo_free_value(jsonValue);
					free(wrapper - offset);
					return -1;
				}
				store_free_value(oldVolume);
			} else {
				struct store_value *volumeValue;
				if((volumeValue = malloc(sizeof(struct store_value))) == NULL) {
					perror("malloc failed");
					free(eventName);
					//argo_free_value(event);
					if(fclose(fmemstream) == EOF) {
						perror("fclose failed");
						return -1;
					}
					argo_free_value(jsonValue);
					free(wrapper - offset);
					return -1;
				}
				volumeValue->type = STORE_DOUBLE_TYPE;
				volumeValue->content.double_value = amountDouble;
				if(store_put(volumeKey, volumeValue)){
					perror("store_put failed");
					free(eventName);
					//argo_free_value(event);
					if(fclose(fmemstream) == EOF) {
						perror("fclose failed");
						return -1;
					}
					argo_free_value(jsonValue);
					free(wrapper - offset);
					return -1;
				}
			}
			free(volumeKey);
		} else if(!strcmp(eventName, "bts:subscription_succeeded")) {
			int initalStoreKeyLen;
			if((initalStoreKeyLen = snprintf(NULL, 0, "%s:%s:%s", wp->wType->name, wp->args[0], "volume")) < 0) {
				perror("Undesired event");
				free(eventName);
				//argo_free_value(event);
				// free(initalStore);
				if(fclose(fmemstream) == EOF) {
					perror("fclose failed");
					return -1;
				}
				argo_free_value(jsonValue);
				free(wrapper - offset);
				return -1;
			}
			char *initalStoreKey;
			if((initalStoreKey = malloc(initalStoreKeyLen + 1)) == NULL) {
				perror("Undesired event");
				free(eventName);
				//argo_free_value(event);
				// free(initalStore);
				if(fclose(fmemstream) == EOF) {
					perror("fclose failed");
					return -1;
				}
				argo_free_value(jsonValue);
				free(wrapper - offset);
				return -1;
			}
			if(sprintf(initalStoreKey, "%s:%s:%s", wp->wType->name, wp->args[0], "volume") < 0) {
				perror("Undesired event");
				free(eventName);
				//argo_free_value(event);
				// free(initalStore);
				if(fclose(fmemstream) == EOF) {
					perror("fclose failed");
					return -1;
				}
				argo_free_value(jsonValue);
				free(wrapper - offset);
				return -1;
			}

			// debug("hi");
			if(!(store_get(initalStoreKey))) {
				// debug("yay");
				struct store_value *initalStore;
				if((initalStore = malloc(sizeof(struct store_value))) == NULL) {
					perror("Undesired event");
					free(eventName);
					free(initalStoreKey);
					//argo_free_value(event);
					if(fclose(fmemstream) == EOF) {
						perror("fclose failed");
						return -1;
					}
					argo_free_value(jsonValue);
					free(wrapper - offset);
					return -1;
				}
				initalStore->type = STORE_DOUBLE_TYPE;
				initalStore->content.double_value = 0.0;


				if(store_put(initalStoreKey, initalStore)) {
					perror("Undesired event");
					free(eventName);
					//argo_free_value(event);
					free(initalStore);
					free(initalStoreKey);
					if(fclose(fmemstream) == EOF) {
						perror("fclose failed");
						return -1;
					}
					argo_free_value(jsonValue);
					free(wrapper - offset);
					return -1;
				}
			}
			free(initalStoreKey);
		} else {
			perror("Undesired event");
			free(eventName);
			//argo_free_value(event);
			if(fclose(fmemstream) == EOF) {
				perror("fclose failed");
				return -1;
			}
			argo_free_value(jsonValue);
			free(wrapper - offset);
			return -1;
		}
		free(eventName);
		//argo_free_value(event);
		// argo_free_value(jsonValue);
	}
	if(fclose(fmemstream) == EOF) {
		perror("fclose failed");
		return -1;
	}
	// if(!argo_value_is_null(jsonValue) && argo_value_is_true(jsonValue)) {
	if(jsonValue) {
		// debug("jsonValue: %p", jsonValue);
		// debug("bye");
		argo_free_value(jsonValue);
	}
	// debug("offset: %d", offset);
	free(wrapper - offset);
	// debug("bye");
	//else to free wrapper and json
	return 0;
}

int bitstamp_watcher_trace(WATCHER *wp, int enable) {
	wp->tracing = enable;
	return 0;
}
