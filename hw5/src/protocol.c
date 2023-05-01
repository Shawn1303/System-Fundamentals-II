#include "protocol.h"
#include "debug.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include "string.h"

/*
 * Send a packet, which consists of a fixed-size header followed by an
 * optional associated data payload.
 *
 * @param fd  The file descriptor on which packet is to be sent.
 * @param hdr  The fixed-size packet header, with multi-byte fields
 *   in network byte order
 * @param data  The data payload, or NULL, if there is none.
 * @return  0 in case of successful transmission, -1 otherwise.
 *   In the latter case, errno is set to indicate the error.
 *
 * All multi-byte fields in the packet are assumed to be in network byte order.
 */
int proto_send_packet(int fd, JEUX_PACKET_HEADER *hdr, void *data) {
	// debug("hi");
	//convert to network byte order
	hdr->size = htons(hdr->size);
	hdr->timestamp_sec = htonl(hdr->timestamp_sec);
	hdr->timestamp_nsec = htonl(hdr->timestamp_nsec);

	//write header
	int bytes_written = 0;
	int bytes_to_write = sizeof(JEUX_PACKET_HEADER);
	char *buffer = (char *)hdr;

	while(bytes_written < bytes_to_write) {
		int results = write(fd, buffer + bytes_written, bytes_to_write - bytes_written);
		if(results < 0) {
			fprintf(stderr, "Error writing header to socket: %s\n", strerror(errno));
			return -1;
			// break;
		} else {
			bytes_written += results;
		}
	}

	//write payload
	if(data && hdr->size > 0) {
		bytes_written = 0;
		bytes_to_write = hdr->size;
		buffer = (char *)data;

		while(bytes_written < bytes_to_write) {
			int results = write(fd, buffer + bytes_written, bytes_to_write - bytes_written);
			if(results < 0) {
				fprintf(stderr, "Error writing data to socket: %s\n", strerror(errno));
				return -1;
			} else {
				bytes_written += results;
			}
		}
	}

	return 0;
}

/*
 * Receive a packet, blocking until one is available.
 *
 * @param fd  The file descriptor from which the packet is to be received.
 * @param hdr  Pointer to caller-supplied storage for the fixed-size
 *   packet header.
 * @param datap  Pointer to a variable into which to store a pointer to any
 *   payload received.
 * @return  0 in case of successful reception, -1 otherwise.  In the
 *   latter case, errno is set to indicate the error.
 *
 * The returned packet has all multi-byte fields in network byte order.
 * If the returned payload pointer is non-NULL, then the caller has the
 * responsibility of freeing that storage.
 */
int proto_recv_packet(int fd, JEUX_PACKET_HEADER *hdr, void **payloadp) {
	// debug("bye");
	//read header
	int bytes_read = 0;
	int bytes_to_read = sizeof(JEUX_PACKET_HEADER);
	char *buffer = (char *)hdr;

	while(bytes_read < bytes_to_read) {
		int results = read(fd, buffer + bytes_read, bytes_to_read - bytes_read);
		if(results < 0) {
			fprintf(stderr, "Error reading header from socket: %s\n", strerror(errno));
			return -1;
		} else if(results == 0) {
			fprintf(stderr, "Socket closed, read EOF in header\n");
			return -1;
		} else {
			bytes_read += results;
		}
	}

	//convert to host byte order
	hdr->size = ntohs(hdr->size);
	hdr->timestamp_sec = ntohl(hdr->timestamp_sec);
	hdr->timestamp_nsec = ntohl(hdr->timestamp_nsec);

	//read payload
	if(hdr->size > 0) {
		if(!(*payloadp = malloc(hdr->size + 1))) {
			fprintf(stderr, "Error allocating memory for payload: %s\n", strerror(errno));
			return -1;
		}
		((char *)(*payloadp))[hdr->size] = '\0'; //null terminate the array

		bytes_read = 0;
		bytes_to_read = hdr->size;
		buffer = (char *)*payloadp;

		while(bytes_read < bytes_to_read) {
			int results = read(fd, buffer + bytes_read, bytes_to_read - bytes_read);
			if(results < 0) {
				fprintf(stderr, "Error reading data: %s\n", strerror(errno));
				return -1;
			} else if(results == 0) {
				fprintf(stderr, "Socket closed, read EOF in payload\n");
				return -1;
			} else {
				bytes_read += results;
			}
		}
	}

	return 0;
}