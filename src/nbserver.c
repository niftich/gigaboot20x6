// Copyright 2016 Google Inc. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files
// (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software,
// and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <stdint.h>
#include <netboot.h>

static uint32_t cookie = 1;

static int io(int s, nbmsg *msg, size_t len, nbmsg *ack) {
	int retries = 5;
	int r;

	msg->magic = NB_MAGIC;
	msg->cookie = cookie++;

	for (;;) {
		r = write(s, msg, len);
		if (r < 0) {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
				continue;
			}
			fprintf(stderr, "\nnbserver: socket write error %d\n", errno);
			return -1;
		}
		r = read(s, ack, 2048);
		if (r < 0) {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
				retries--;
				if (retries > 0) {
					fprintf(stderr, "T");
					continue;
				}
				fprintf(stderr, "\nnbserver: timed out\n");
			} else {
				fprintf(stderr, "\nnbserver: socket read error %d\n", errno);
			}
			return -1;
		}
		if (r < sizeof(nbmsg)) {
			fprintf(stderr, "Z");
			continue;
		}
		if (ack->magic != NB_MAGIC) {
			fprintf(stderr, "?");
			continue;
		}
		if (ack->cookie != msg->cookie) {
			fprintf(stderr, "C");
			continue;
		}
		if (ack->arg != msg->arg) {
			fprintf(stderr, "A");
			continue;
		}
		if (ack->cmd == NB_ACK) return 0;
		return -1;
	}
}

static void xfer(struct sockaddr_in6 *addr, const char *fn) {
	char msgbuf[2048];
	char ackbuf[2048];
	char tmp[INET6_ADDRSTRLEN];
	struct timeval tv;
	nbmsg *msg = (void*) msgbuf;
	nbmsg *ack = (void*) ackbuf;
	FILE *fp;
	int s, r;
	int count = 0;

	if ((fp = fopen(fn, "rb")) == NULL) {
		return;
	}
	if ((s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		fprintf(stderr, "nbserver: cannot create socket %d\n", errno);
		goto done;
	}
	tv.tv_sec = 0;
	tv.tv_usec = 250 * 1000;
	setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (connect(s, (void*) addr, sizeof(*addr)) < 0) {
		fprintf(stderr, "nbserver: cannot connect to [%s]%d\n",
			inet_ntop(AF_INET6, &addr->sin6_addr, tmp, sizeof(tmp)),
			ntohs(addr->sin6_port));
		goto done;
	}

	msg->cmd = NB_SEND_FILE;
	msg->arg = 0;
	strcpy((void*) msg->data, "kernel.bin");
	if (io(s, msg, sizeof(nbmsg) + sizeof("kernel.bin"), ack)) {
		fprintf(stderr, "nbserver: failed to start transfer\n");
		goto done;
	}

	msg->cmd = NB_DATA;
	msg->arg = 0;
	do {
		r = fread(msg->data, 1, 1024, fp);
		if (r == 0) {
			if (ferror(fp)) {
				fprintf(stderr, "\nnbserver: error: reading '%s'\n", fn);
				goto done;
			}
			break;
		}
		count += r;
		if (count >= (32*1024)) {
			count = 0;
			fprintf(stderr, "#");
		}
		if (io(s, msg, sizeof(nbmsg) + r, ack)) {
			fprintf(stderr, "\nnbserver: error: sending '%s'\n", fn);
			goto done;
		}
		msg->arg += r;
	} while (r != 0);

	msg->cmd = NB_BOOT;
	msg->arg = 0;
	if (io(s, msg, sizeof(nbmsg), ack)) {
		fprintf(stderr, "\nnbserver: failed to send boot command\n");
	} else {
		fprintf(stderr, "\nnbserver: sent boot command\n");
	}
done:
	if (s >= 0) close(s);
	if (fp != NULL) fclose(fp);
}

void usage(void) {
	fprintf(stderr,
		"usage:   nbserver [ <option> ]* <filename>\n"
		"\n"
		"options: -1  only boot once, then exit\n"
		);
	exit(1);
}
		
int main(int argc, char **argv) {
	struct sockaddr_in6 addr;
	char tmp[INET6_ADDRSTRLEN];
	int r, s, n = 1;
	const char *fn = NULL;
	int once = 0;

	while (argc > 1) {
		if (argv[1][0] != '-') {
			if (fn != NULL) usage();
			fn = argv[1];
		} else if (!strcmp(argv[1], "-1")) {
			once = 1;
		} else {
			usage();
		}
		argc--;
		argv++;
	}
	if (fn == NULL) {
		usage();
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(NB_ADVERT_PORT);

	s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (s < 0) {
		fprintf(stderr, "nbserver: cannot create socket %d\n", s);
		return -1;
	}
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n));
	if ((r = bind(s, (void*) &addr, sizeof(addr))) < 0) {
		fprintf(stderr, "nbserver: cannot bind to [%s]%d %d\n",
			inet_ntop(AF_INET6, &addr.sin6_addr, tmp, sizeof(tmp)),
			ntohs(addr.sin6_port), r);
		return -1;
	}

	fprintf(stderr, "nbserver: listening on [%s]%d\n",
		inet_ntop(AF_INET6, &addr.sin6_addr, tmp, sizeof(tmp)),
		ntohs(addr.sin6_port));
	for (;;) {
		struct sockaddr_in6 ra;
		socklen_t rlen;
		char buf[4096];
		nbmsg *msg = (void*) buf;
		rlen = sizeof(ra);
		r = recvfrom(s, buf, 4096, 0, (void*) &ra, &rlen);
		if (r < 0) {
			fprintf(stderr, "nbserver: socket read error %d\n", r);
			break;
		}
		if (r < sizeof(nbmsg)) continue;
		if ((ra.sin6_addr.s6_addr[0] != 0xFE) || (ra.sin6_addr.s6_addr[1] != 0x80)) {
			fprintf(stderr, "ignoring non-link-local message\n");
			continue;
		}
		if (msg->magic != NB_MAGIC) continue;
		if (msg->cmd != NB_ADVERTISE) continue;
		fprintf(stderr, "nbserver: got beacon from [%s]%d\n",
			inet_ntop(AF_INET6, &ra.sin6_addr, tmp, sizeof(tmp)),
			ntohs(ra.sin6_port));
		fprintf(stderr, "nbserver: sending '%s'...\n", fn);
		xfer(&ra, fn);
		if (once) {
			break;
		}
	}

	return 0;
}

