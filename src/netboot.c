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

#include <stdio.h>
#include <string.h>

#include <netboot.h>
#include <netifc.h>
#include <inet6.h>

static uint32_t last_cookie = 0;
static uint32_t last_cmd = 0;
static uint32_t last_arg = 0;
static uint32_t last_ack_cmd = 0;
static uint32_t last_ack_arg = 0;

static uint8_t *nb_buffer;
static uint32_t nb_bufsize = 0;
static uint32_t nb_offset = 0;

static int nb_boot_now = 0;
static int nb_active = 0;

void udp6_recv(void *data, size_t len,
	const ip6_addr *daddr, uint16_t dport,
	const ip6_addr *saddr, uint16_t sport) {
	nbmsg *msg = data;
	nbmsg ack;

	if (dport != NB_SERVER_PORT) return;

	if (len < sizeof(nbmsg)) return;
	len -= sizeof(nbmsg);

	//printf("netboot: MSG %08x %08x %08x %08x datalen %d\n",
	//	msg->magic, msg->cookie, msg->cmd, msg->arg, len);

	if ((last_cookie == msg->cookie) &&
		(last_cmd == msg->cmd) && (last_arg = msg->arg)) {
		// host must have missed the ack. resend
		ack.magic = NB_MAGIC;
		ack.cookie = last_cookie;
		ack.cmd = last_ack_cmd;
		ack.arg = last_ack_arg;
		goto transmit;
	}

	ack.cmd = NB_ACK;
	ack.arg = 0;

	switch (msg->cmd) {
	case NB_COMMAND:
		if (len == 0) return;
		msg->data[len - 1] = 0;
		break;
	case NB_SEND_FILE:
		if (len == 0) return;
		msg->data[len - 1] = 0;		
		nb_offset = 0;
		printf("netboot: Receive File...\n");
		break;
	case NB_DATA:
		if (msg->arg != nb_offset) return;
		if (nb_buffer == 0) return;
		ack.arg = msg->arg;
		if ((nb_offset + len) > nb_bufsize) {
			ack.cmd = NB_ERROR_TOO_LARGE;
		} else {
			memcpy(nb_buffer + nb_offset, msg->data, len);
			nb_offset += len;
			ack.cmd = NB_ACK;
		}
		break;
	case NB_BOOT:
		nb_boot_now = 1;
		printf("netboot: Boot Kernel...\n");
		break;
	default:
		ack.cmd = NB_ERROR_BAD_CMD;
		ack.arg = 0;
	}

	last_cookie = msg->cookie;
	last_cmd = msg->cmd;
	last_arg = msg->arg;
	last_ack_cmd = ack.cmd;
	last_ack_arg = ack.arg;

	ack.cookie = msg->cookie;
	ack.magic = NB_MAGIC;
transmit:
	nb_active = 1;
	udp6_send(&ack, sizeof(ack), saddr, sport, NB_SERVER_PORT);
}

static char advertise_data[] = 
	"version\00.1\0"
	"serialno\0unknown\0"
	"board\0unknown\0";

static void advertise(void) {
	uint8_t buffer[256];
	nbmsg *msg = (void*) buffer;
	msg->magic = NB_MAGIC;
	msg->cookie = 0;
	msg->cmd = NB_ADVERTISE;
	msg->arg = 0;
	memcpy(msg->data, advertise_data, sizeof(advertise_data));
	udp6_send(buffer, sizeof(nbmsg) + sizeof(advertise_data),
		&ip6_ll_all_nodes, NB_ADVERT_PORT, NB_SERVER_PORT);
}

#define FAST_TICK 100
#define SLOW_TICK 1000

int netboot_init(void *buf, size_t len) {
	if (netifc_open()) {
		printf("netboot: Failed to open network interface\n");
		return -1;
	}

	nb_buffer = buf;
	nb_bufsize = len;
	return 0;
}

static int nb_fastcount = 0;
static int nb_online = 0;

int netboot_poll(void) {
	if (netifc_active()) {
		if (nb_online == 0) {
			printf("netboot: interface online\n");
			nb_online = 1;
			nb_fastcount = 20;
			netifc_set_timer(FAST_TICK);
			advertise();
		}
	} else {
		if (nb_online == 1) {
			printf("netboot: interface offline\n");
			nb_online = 0;
		}
		return 0;
	}
	if (netifc_timer_expired()) {
		if (nb_fastcount) {
			nb_fastcount--;
			netifc_set_timer(FAST_TICK);
		} else {
			netifc_set_timer(SLOW_TICK);
		}
		if (nb_active) {
			// don't advertise if we're in a transfer
			nb_active = 0;
		} else {
			advertise();
		}
	}

	netifc_poll();

	if (nb_boot_now) {
		nb_boot_now = 0;
		return nb_offset;
	} else {
		return 0;
	}
}

void netboot_close(void) {
	netifc_close();
}
