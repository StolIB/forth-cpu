/**@file uart.c
 * @brief This is a program for transferring data via the UART to the target
 * board, it is currently Linux only (not tested on other Unixen). It sends
 * a byte via a UART (115200 baud, 8 bits, 1 stop bit). 
 *
 * @author Richard James Howe
 * @copyright Richard James Howe (c) 2017
 * @license MIT
 */

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

static int open_tty(const char * port)
{
	int fd;
	errno = 0;
	fd = open(port, O_RDWR | O_NOCTTY);
	if (fd == -1) {
		fprintf(stderr, "%s unable to open '%s': %s\n", __func__, port, strerror(errno));
		exit(EXIT_FAILURE);
	}
	return fd;
}

static void *stdin_to_uart(void *x)
{
	int fd = *(int*)x;
	int c = 0;
	unsigned char c1 = 0;
	while(EOF != (c = fgetc(stdin))) {
		c1 = c;
		errno = 0;
		if (write(fd, &c1, 1) != 1) {
			fprintf(stderr, "write error:%s\n", strerror(errno));
			return NULL;
		}

	}
	exit(EXIT_SUCCESS);
	return NULL;
}

static void *uart_to_stdout(void *x)
{
	int fd = *(int*)x;
	unsigned char c = 0;

	for(;;) {
		if(1 == read(fd, &c, 1))
			write(1, &c, 1);
	}
	return NULL;
}

int main(int argc, char **argv)
{
	int fd = -1;
	const char *port = "/dev/ttyUSB0";
	struct termios options;
	pthread_t p1, p2;

	if (argc == 2) {
		port = argv[1];
	} else if (argc != 1) {
		fprintf(stderr, "usage: %s /dev/ttyUSBX <file\n", argv[0]);
		return -1;
	}
	fd = open_tty(port);

	errno = 0;
	if (tcgetattr(fd, &options) < 0) {
		fprintf(stderr, "failed to get terminal options on fd %d: %s\n", fd, strerror(errno));
		return -1;
	}

	cfsetispeed(&options, B115200);
	cfsetospeed(&options, B115200);

	cfmakeraw(&options);
	options.c_cflag |= (CLOCAL | CREAD); /* Enable the receiver and set local mode */
	options.c_cflag &= ~CSTOPB;	     /* 1 stopbit */
	options.c_cflag &= ~CRTSCTS;         /* Disable hardware flow control */
	// options.c_cc[VMIN]  = 0;
	// options.c_cc[VTIME] = 1;             /* Timeout read after 1 second */

	errno = 0;
	if (tcsetattr(fd, TCSANOW, &options) < 0) {
		fprintf(stderr, "failed to set terminal options on fd %d: %s\n", fd, strerror(errno));
		exit(EXIT_FAILURE);
	}

	errno = 0;
	if(pthread_create(&p1, NULL, uart_to_stdout, &fd)) {
		fprintf(stderr, "failed to create thread 1: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	errno = 0;
	if(pthread_create(&p2, NULL, stdin_to_uart, &fd)) {
		fprintf(stderr, "failed to create thread 2: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if(pthread_join(p2, NULL)) {
		fprintf(stderr, "Error joining thread\n");
		return -2;
	}

	close(fd);
	return 0;
}
