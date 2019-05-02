/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2017 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "usb_dev.h"
#include "usb_serial.h"
#include "core_pins.h"// for delay()
//#include "HardwareSerial.h"
#include <string.h> // for memcpy()

#include "debug/printf.h"
#include "core_pins.h"

// defined by usb_dev.h -> usb_desc.h
#if defined(CDC_STATUS_INTERFACE) && defined(CDC_DATA_INTERFACE)
//#if F_CPU >= 20000000

uint32_t usb_cdc_line_coding[2];
volatile uint32_t usb_cdc_line_rtsdtr_millis;
volatile uint8_t usb_cdc_line_rtsdtr=0;
volatile uint8_t usb_cdc_transmit_flush_timer=0;

//static usb_packet_t *rx_packet=NULL;
//static usb_packet_t *tx_packet=NULL;
static volatile uint8_t tx_noautoflush=0;

#define TRANSMIT_FLUSH_TIMEOUT	5   /* in milliseconds */



#define RX_NUM  3
static transfer_t rx_transfer[RX_NUM] __attribute__ ((used, aligned(32)));
static uint8_t rx_buffer[RX_NUM * CDC_RX_SIZE];
static uint16_t rx_count[RX_NUM];
static uint16_t rx_index[RX_NUM];

static void rx_event(transfer_t *t)
{
	int len = CDC_RX_SIZE - ((t->status >> 16) & 0x7FFF);
	int index = t->callback_param;
	printf("rx event, len=%d, i=%d\n", len, index);
	rx_count[index] = len;
	rx_index[index] = 0;
}

void usb_serial_reset(void)
{
	printf("usb_serial_reset\n");
	// deallocate all transfer descriptors
}

void usb_serial_configure(void)
{
	printf("usb_serial_configure\n");
	usb_config_tx(CDC_ACM_ENDPOINT, CDC_ACM_SIZE, 0, NULL);
	usb_config_rx(CDC_RX_ENDPOINT, CDC_RX_SIZE, 0, rx_event);
	usb_config_tx(CDC_TX_ENDPOINT, CDC_TX_SIZE, 0, NULL);
	usb_prepare_transfer(rx_transfer + 0, rx_buffer + 0, CDC_RX_SIZE, 0);
	usb_receive(CDC_RX_ENDPOINT, rx_transfer + 0);
}


// get the next character, or -1 if nothing received
int usb_serial_getchar(void)
{
	if (rx_index[0] < rx_count[0]) {
		int c = rx_buffer[rx_index[0]++];
		if (rx_index[0] >= rx_count[0]) {
			// reschedule transfer
			usb_prepare_transfer(rx_transfer + 0, rx_buffer + 0, CDC_RX_SIZE, 0);
			usb_receive(CDC_RX_ENDPOINT, rx_transfer + 0);
		}
		return c;
	}
#if 0
	unsigned int i;
	int c;

	if (!rx_packet) {
		if (!usb_configuration) return -1;
		rx_packet = usb_rx(CDC_RX_ENDPOINT);
		if (!rx_packet) return -1;
	}
	i = rx_packet->index;
	c = rx_packet->buf[i++];
	if (i >= rx_packet->len) {
		usb_free(rx_packet);
		rx_packet = NULL;
	} else {
		rx_packet->index = i;
	}
	return c;
#endif
	return -1;
}

// peek at the next character, or -1 if nothing received
int usb_serial_peekchar(void)
{
#if 0
	if (!rx_packet) {
		if (!usb_configuration) return -1;
		rx_packet = usb_rx(CDC_RX_ENDPOINT);
		if (!rx_packet) return -1;
	}
	if (!rx_packet) return -1;
	return rx_packet->buf[rx_packet->index];
#endif
	return -1;
}

// number of bytes available in the receive buffer
int usb_serial_available(void)
{
	return rx_count[0] - rx_index[0];
#if 0
	int count;
	count = usb_rx_byte_count(CDC_RX_ENDPOINT);
	if (rx_packet) count += rx_packet->len - rx_packet->index;
	return count;
#endif
	return 0;
}

// read a block of bytes to a buffer
int usb_serial_read(void *buffer, uint32_t size)
{
#if 0
	uint8_t *p = (uint8_t *)buffer;
	uint32_t qty, count=0;

	while (size) {
		if (!usb_configuration) break;
		if (!rx_packet) {
			rx:
			rx_packet = usb_rx(CDC_RX_ENDPOINT);
			if (!rx_packet) break;
			if (rx_packet->len == 0) {
				usb_free(rx_packet);
				goto rx;
			}
		}
		qty = rx_packet->len - rx_packet->index;
		if (qty > size) qty = size;
		memcpy(p, rx_packet->buf + rx_packet->index, qty);
		p += qty;
		count += qty;
		size -= qty;
		rx_packet->index += qty;
		if (rx_packet->index >= rx_packet->len) {
			usb_free(rx_packet);
			rx_packet = NULL;
		}
	}
	return count;
#endif
	return 0;
}

// discard any buffered input
void usb_serial_flush_input(void)
{
#if 0
	usb_packet_t *rx;

	if (!usb_configuration) return;
	if (rx_packet) {
		usb_free(rx_packet);
		rx_packet = NULL;
	}
	while (1) {
		rx = usb_rx(CDC_RX_ENDPOINT);
		if (!rx) break;
		usb_free(rx);
	}
#endif
}

// Maximum number of transmit packets to queue so we don't starve other endpoints for memory
//#define TX_PACKET_LIMIT 8

// When the PC isn't listening, how long do we wait before discarding data?  If this is
// too short, we risk losing data during the stalls that are common with ordinary desktop
// software.  If it's too long, we stall the user's program when no software is running.
#define TX_TIMEOUT_MSEC 70

/*#if F_CPU == 240000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 1600)
#elif F_CPU == 216000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 1440)
#elif F_CPU == 192000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 1280)
#elif F_CPU == 180000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 1200)
#elif F_CPU == 168000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 1100)
#elif F_CPU == 144000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 932)
#elif F_CPU == 120000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 764)
#elif F_CPU == 96000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 596)
#elif F_CPU == 72000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 512)
#elif F_CPU == 48000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 428)
#elif F_CPU == 24000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 262)
#endif */

// When we've suffered the transmit timeout, don't wait again until the computer
// begins accepting data.  If no software is running to receive, we'll just discard
// data as rapidly as Serial.print() can generate it, until there's something to
// actually receive it.
//static uint8_t transmit_previous_timeout=0;


// transmit a character.  0 returned on success, -1 on error
int usb_serial_putchar(uint8_t c)
{
	return usb_serial_write(&c, 1);
}


static transfer_t txtransfer __attribute__ ((used, aligned(32)));
static uint8_t txbuffer[1024];
//static uint8_t txbuffer1[1024];
//static uint8_t txbuffer2[1024];
//static uint8_t txstate=0;

int usb_serial_write(const void *buffer, uint32_t size)
{
	if (!usb_configuration) return 0;
	if (size > sizeof(txbuffer)) size = sizeof(txbuffer);
	int count=0;
	while (1) {
		uint32_t status = usb_transfer_status(&txtransfer);
		if (count > 2000) {
			printf("status = %x\n", status);
			//while (1) ;
		}
		if (!(status & 0x80)) break;
		count++;
		//if (count > 50) break; // TODO: proper timout?
		// TODO: check for USB offline
	}
	memcpy(txbuffer, buffer, size);
	usb_prepare_transfer(&txtransfer, txbuffer, size, 0);
	usb_transmit(CDC_TX_ENDPOINT, &txtransfer);

#if 0
	uint32_t ret = size;
	uint32_t len;
	uint32_t wait_count;
	const uint8_t *src = (const uint8_t *)buffer;
	uint8_t *dest;

	tx_noautoflush = 1;
	while (size > 0) {
		if (!tx_packet) {
			wait_count = 0;
			while (1) {
				if (!usb_configuration) {
					tx_noautoflush = 0;
					return -1;
				}
				if (usb_tx_packet_count(CDC_TX_ENDPOINT) < TX_PACKET_LIMIT) {
					tx_noautoflush = 1;
					tx_packet = usb_malloc();
					if (tx_packet) break;
					tx_noautoflush = 0;
				}
				if (++wait_count > TX_TIMEOUT || transmit_previous_timeout) {
					transmit_previous_timeout = 1;
					return -1;
				}
				yield();
			}
		}
		transmit_previous_timeout = 0;
		len = CDC_TX_SIZE - tx_packet->index;
		if (len > size) len = size;
		dest = tx_packet->buf + tx_packet->index;
		tx_packet->index += len;
		size -= len;
		while (len-- > 0) *dest++ = *src++;
		if (tx_packet->index >= CDC_TX_SIZE) {
			tx_packet->len = CDC_TX_SIZE;
			usb_tx(CDC_TX_ENDPOINT, tx_packet);
			tx_packet = NULL;
		}
		usb_cdc_transmit_flush_timer = TRANSMIT_FLUSH_TIMEOUT;
	}
	tx_noautoflush = 0;
	return ret;
#endif
	return 0;
}

int usb_serial_write_buffer_free(void)
{
#if 0
	uint32_t len;

	tx_noautoflush = 1;
	if (!tx_packet) {
		if (!usb_configuration ||
		  usb_tx_packet_count(CDC_TX_ENDPOINT) >= TX_PACKET_LIMIT ||
		  (tx_packet = usb_malloc()) == NULL) {
			tx_noautoflush = 0;
			return 0;
		}
	}
	len = CDC_TX_SIZE - tx_packet->index;
	// TODO: Perhaps we need "usb_cdc_transmit_flush_timer = TRANSMIT_FLUSH_TIMEOUT"
	// added here, so the SOF interrupt can't take away the available buffer
	// space we just promised the user could write without blocking?
	// But does this come with other performance downsides?  Could it lead to
	// buffer data never actually transmitting in some usage cases?  More
	// investigation is needed.
	// https://github.com/PaulStoffregen/cores/issues/10#issuecomment-61514955
	tx_noautoflush = 0;
	return len;
#endif
	return 0;
}

void usb_serial_flush_output(void)
{
#if 0
	if (!usb_configuration) return;
	tx_noautoflush = 1;
	if (tx_packet) {
		usb_cdc_transmit_flush_timer = 0;
		tx_packet->len = tx_packet->index;
		usb_tx(CDC_TX_ENDPOINT, tx_packet);
		tx_packet = NULL;
	} else {
		usb_packet_t *tx = usb_malloc();
		if (tx) {
			usb_cdc_transmit_flush_timer = 0;
			usb_tx(CDC_TX_ENDPOINT, tx);
		} else {
			usb_cdc_transmit_flush_timer = 1;
		}
	}
	tx_noautoflush = 0;
#endif
}

void usb_serial_flush_callback(void)
{
#if 0
	if (tx_noautoflush) return;
	if (tx_packet) {
		tx_packet->len = tx_packet->index;
		usb_tx(CDC_TX_ENDPOINT, tx_packet);
		tx_packet = NULL;
	} else {
		usb_packet_t *tx = usb_malloc();
		if (tx) {
			usb_tx(CDC_TX_ENDPOINT, tx);
		} else {
			usb_cdc_transmit_flush_timer = 1;
		}
	}
#endif
}



//#endif // F_CPU
#endif // CDC_STATUS_INTERFACE && CDC_DATA_INTERFACE
