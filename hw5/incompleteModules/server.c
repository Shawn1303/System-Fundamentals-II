#include "server.h"
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "debug.h"

/*
 * Client registry that should be used to track the set of
 * file descriptors of currently connected clients.
 */
// extern CLIENT_REGISTRY *client_registry;
CLIENT_REGISTRY *client_registry;


/*
 * Thread function for the thread that handles a particular client.
 *
 * @param  Pointer to a variable that holds the file descriptor for
 * the client connection.  This pointer must be freed once the file
 * descriptor has been retrieved.
 * @return  NULL
 *
 * This function executes a "service loop" that receives packets from
 * the client and dispatches to appropriate functions to carry out
 * the client's requests.  It also maintains information about whether
 * the client has logged in or not.  Until the client has logged in,
 * only LOGIN packets will be honored.  Once a client has logged in,
 * LOGIN packets will no longer be honored, but other packets will be.
 * The service loop ends when the network connection shuts down and
 * EOF is seen.  This could occur either as a result of the client
 * explicitly closing the connection, a timeout in the network causing
 * the connection to be closed, or the main thread of the server shutting
 * down the connection as part of graceful termination.
 */
void *jeux_client_service(void *arg) {
	int fd = *(int *)arg;
	free(arg);

	pthread_detach(pthread_self());

	// CLIENT_REGISTRY *cr = client_registry;

	CLIENT *client;
	if(!(client = creg_register(client_registry, fd))) {
		error("Failed to register client");
		close(fd);
		return NULL;
	}

	JEUX_PACKET_HEADER *hdr;
	if(!(hdr = malloc(sizeof(JEUX_PACKET_HEADER)))) {
		error("Failed to allocate memory for packet header");
		creg_unregister(client_registry, client);
		close(fd);
		return NULL;
	}

	memset(hdr, 0, sizeof(JEUX_PACKET_HEADER));

	void *payload = NULL;

	while(!(proto_recv_packet(fd, hdr, &payload))) {
		switch(hdr->type) {
			case JEUX_LOGIN_PKT:
			case JEUX_USERS_PKT:
			case JEUX_INVITE_PKT:
			case JEUX_REVOKE_PKT:
			case JEUX_ACCEPT_PKT:
			case JEUX_DECLINE_PKT:
			case JEUX_MOVE_PKT:
			case JEUX_RESIGN_PKT:
		}
	}

	return NULL;
}