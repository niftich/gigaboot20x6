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

#include <efi.h>
#include <efilib.h>
#include <string.h>
#include <stdio.h>

#include <goodies.h>

#include <netifc.h>
#include <inet6.h>

static EFI_SIMPLE_NETWORK *snp;

#define MAX_FILTER 8
static EFI_MAC_ADDRESS mcast_filters[MAX_FILTER];
static unsigned mcast_filter_count = 0;

#define NUM_BUFFER_PAGES	8
#define ETH_BUFFER_SIZE		1516
#define ETH_HEADER_SIZE		16
#define ETH_BUFFER_MAGIC	0x424201020304A7A7UL

typedef struct eth_buffer_t eth_buffer;
struct eth_buffer_t {
	uint64_t magic;
	eth_buffer *next;
	uint8_t data[0];
};

static EFI_PHYSICAL_ADDRESS eth_buffers_base = 0;
static eth_buffer *eth_buffers = NULL;

void *eth_get_buffer(size_t sz) {
	eth_buffer *buf;
	if (sz > ETH_BUFFER_SIZE) {
		return NULL;
	}
	if (eth_buffers == NULL) {
		return NULL;
	}
	buf = eth_buffers;
	eth_buffers = buf->next;
	buf->next = NULL;
	return buf->data;
}

void eth_put_buffer(void *data) {
	eth_buffer *buf = (void*) (((uint64_t) data) & (~2047));

	if (buf->magic != ETH_BUFFER_MAGIC) {
		printf("fatal: eth buffer %p (from %p) bad magic %lx\n", buf, data, buf->magic);
		for (;;) ;
	}
	buf->next = eth_buffers;
	eth_buffers = buf;
}

int eth_send(void *data, size_t len) {
	EFI_STATUS r;

	if ((r = snp->Transmit(snp, 0, len, (void*) data, NULL, NULL, NULL))) {
		eth_put_buffer(data);
		return -1;
	} else {
		return 0;
	}
}

void eth_dump_status(void) {
	printf("State/HwAdSz/HdrSz/MaxSz %d %d %d %d\n",
		snp->Mode->State, snp->Mode->HwAddressSize,
		snp->Mode->MediaHeaderSize, snp->Mode->MaxPacketSize);
	printf("RcvMask/RcvCfg/MaxMcast/NumMcast %d %d %d %d\n",
		snp->Mode->ReceiveFilterMask, snp->Mode->ReceiveFilterSetting,
		snp->Mode->MaxMCastFilterCount, snp->Mode->MCastFilterCount);
	UINT8 *x = snp->Mode->CurrentAddress.Addr;
	printf("MacAddr %02x:%02x:%02x:%02x:%02x:%02x\n",
		x[0], x[1], x[2], x[3], x[4], x[5]);
	printf("SetMac/MultiTx/LinkDetect/Link %d %d %d %d\n",
		snp->Mode->MacAddressChangeable, snp->Mode->MultipleTxSupported,
		snp->Mode->MediaPresentSupported, snp->Mode->MediaPresent);
}

int eth_add_mcast_filter(const mac_addr *addr) {
	if (mcast_filter_count >= MAX_FILTER) return -1;
	if (mcast_filter_count >= snp->Mode->MaxMCastFilterCount) return -1;
	memcpy(mcast_filters + mcast_filter_count, addr, ETH_ADDR_LEN);
	mcast_filter_count++;
	return 0;
}

static EFI_EVENT net_timer = NULL;

#define TIMER_MS(n) (((uint64_t) (n)) * 10000UL)

void netifc_set_timer(uint32_t ms) {
	if (net_timer == 0) {
		return;
	}
	gBS->SetTimer(net_timer, TimerRelative, TIMER_MS(ms));
}

int netifc_timer_expired(void) {
	if (net_timer == 0) {
		return 0;
	}
	if (gBS->CheckEvent(net_timer) == EFI_SUCCESS) {
		return 1;
	}
	return 0;
}

int netifc_open(void) {
	EFI_BOOT_SERVICES *bs = gSys->BootServices;
	EFI_HANDLE h[32];
	EFI_STATUS r;
	int i, j;
	UINTN sz;

	bs->CreateEvent(EVT_TIMER, TPL_CALLBACK, NULL, NULL, &net_timer);

	sz = sizeof(h);
	r = bs->LocateHandle(ByProtocol, &SimpleNetworkProtocol, NULL, &sz, h);
	if (r != EFI_SUCCESS) {
		printf("Failed to locate SNP handle(s) %ld\n", r);
		return -1;
	}

	r = bs->OpenProtocol(h[0], &SimpleNetworkProtocol, (void**) &snp, gImg, NULL,
		EFI_OPEN_PROTOCOL_EXCLUSIVE);
	if (r) {
		printf("Failed to open SNP exclusively %ld\n", r);
		return -1;
	}

	if (snp->Mode->State != EfiSimpleNetworkStarted) {
		snp->Start(snp);
		if (snp->Mode->State != EfiSimpleNetworkStarted) {
			printf("Failed to start SNP\n");
			return -1;
		}
		r = snp->Initialize(snp, 32768, 32768);
		if (r) {
			printf("Failed to initialize SNP\n");
			return -1;
		}
	}

	if (bs->AllocatePages(AllocateAnyPages, EfiLoaderData, NUM_BUFFER_PAGES, &eth_buffers_base)) {
		printf("Failed to allocate net buffers\n");
		return -1;
	}

	uint8_t *ptr = (void*) eth_buffers_base;
	for (r = 0; r < (NUM_BUFFER_PAGES * 2); r++) {
		eth_buffer *buf = (void*) ptr;
		buf->magic = ETH_BUFFER_MAGIC;
		eth_put_buffer(buf);
		ptr += 2048;
	}

	ip6_init(snp->Mode->CurrentAddress.Addr);

	r = snp->ReceiveFilters(snp,
		EFI_SIMPLE_NETWORK_RECEIVE_UNICAST |
		EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST,
		0, 0, mcast_filter_count, (void*) mcast_filters);
	if (r) {
		printf("Failed to install multicast filters %lx\n", r);
		return -1;
	}

	eth_dump_status();

	if (snp->Mode->MCastFilterCount != mcast_filter_count) {
		printf("OOPS: expected %d filters, found %d\n",
			mcast_filter_count, snp->Mode->MCastFilterCount);
		goto force_promisc;
	}
	for (i = 0; i < mcast_filter_count; i++) {
		//uint8_t *m = (void*) &mcast_filters[i];
		//printf("i=%d %02x %02x %02x %02x %02x %02x\n", i, m[0], m[1], m[2], m[3], m[4], m[5]);
		for (j = 0; j < mcast_filter_count; j++) {
			//m = (void*) &snp->Mode->MCastFilter[j];
			//printf("j=%d %02x %02x %02x %02x %02x %02x\n", j, m[0], m[1], m[2], m[3], m[4], m[5]);
			if (!memcmp(mcast_filters + i, &snp->Mode->MCastFilter[j], 6)) {
				goto found_it;
			}
		}
		printf("OOPS: filter #%d missing\n", i);
		goto force_promisc;
	found_it:
		;
	}

	return 0;

force_promisc:
	r = snp->ReceiveFilters(snp,
		EFI_SIMPLE_NETWORK_RECEIVE_UNICAST |
		EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS |
		EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST,
		0, 0, 0, NULL);
	if (r) {
		printf("Failed to set promiscuous mode %lx\n", r);
		return -1;
	}
	return 0;
}

void netifc_close(void) {
	gBS->SetTimer(net_timer, TimerCancel, 0);
	gBS->CloseEvent(net_timer);
	snp->Shutdown(snp);
	snp->Stop(snp);
}

int netifc_active(void) {
	return (snp != 0);
}

void netifc_poll(void) {
	UINT8 data[1514];
	EFI_STATUS r;
	UINTN hsz, bsz;
	UINT32 irq;
	VOID *txdone;

	if ((r = snp->GetStatus(snp, &irq, &txdone))) {
		return;
	}

	if (txdone) {
		eth_put_buffer(txdone);
	}
		
	if (irq & EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT) {
		hsz = 0;
		bsz = sizeof(data);
		r = snp->Receive(snp, &hsz, &bsz, data, NULL, NULL, NULL);
		if (r != EFI_SUCCESS) {
			return;
		}
#if TRACE
		printf("RX %02x:%02x:%02x:%02x:%02x:%02x < %02x:%02x:%02x:%02x:%02x:%02x %02x%02x %d\n",
			data[0], data[1], data[2], data[3], data[4], data[5],
			data[6], data[7], data[8], data[9], data[10], data[11],
			data[12], data[13], (int) (bsz - hsz));
#endif
		eth_recv(data, bsz);
	}
}


