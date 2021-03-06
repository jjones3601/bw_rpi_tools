/*
 * bw_i2c_lcd.c. 
 *
 * Control the BitWizard I2C-LCD expansion boards. 
 *
 * based on: 
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 *
 *
 *
 * Compile on raspberry pi with 
 *
 * gcc -Wall -O2 bw_i2c_lcd.c -o bw_i2c_lcd 
 *
 */


#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <linux/i2c-dev.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))


static const char *device = "/dev/i2c-0";
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 500000;
static uint16_t delay = 15;
static int cls = 0;
static int reg = -1;
static int val = -1;
static int x = -1, y = -1, addr=0x82; 
static int backlight = -1;
static int contrast = -1;
static int text = 0;


static void pabort(const char *s)
{
  perror(s);
  abort();
}

void transfer (int fd, int len, char *buf)
{
   write (fd, buf+1, len-1);
}


static void send_text (int fd, char *str) 
{
  char *buf; 
  int l;
  l = strlen (str);
  buf = malloc (l + 5); 
  buf[0] = addr; 
  buf[1] = 0; 
  strcpy (buf+2, str); 
  transfer (fd, l+2, buf);
  free (buf);
}


static void set_reg_value8 (int fd, int reg, int val)
{
  char buf[5]; 
  buf[0] = addr;
  buf[1] = reg;
  buf[2] = val;
  transfer (fd, 3, buf);
}



static void print_usage(const char *prog)
{
  printf("Usage: %s [-DsbdlHOLC3]\n", prog);
  puts("  -D --device   device to use (default /dev/spidev1.1)\n"
       "  -s --speed    max speed (Hz)\n"
       "  -d --delay    delay (usec)\n"
       "  -b --bpw      bits per word \n"
       "  -l --loop     loopback\n"
       "  -H --cpha     clock phase\n"
       "  -O --cpol     clock polarity\n"
       "  -L --lsb      least significant bit first\n"
       "  -C --cs-high  chip select active high\n"
       "  -3 --3wire    SI/SO signals shared\n");
  exit(1);
}

static const struct option lopts[] = {

  // SPI options. 
  { "device",  1, 0, 'D' },
  { "speed",   1, 0, 's' },
  { "delay",   1, 0, 'd' },

  // text display options. 
  { "reg",       1, 0, 'r' },
  { "val",       1, 0, 'v' },
  { "pos",       1, 0, 'p' },
  { "addr",      1, 0, 'a' },
  { "text",      1, 0, 't' },
  { "ptext",     1, 0, 'T' },
  { "backlight", 1, 0, 'b' },
  { "contrast",  1, 0, 'c' },
  { "file",      1, 0, 'f' },

  { "cls",       0, 0, 'C' },
  
  { NULL, 0, 0, 0 },
};



static int parse_opts(int argc, char *argv[])
{
  while (1) {
    int c;

    c = getopt_long(argc, argv, "D:s:d:r:v:p:a:tT:b:c:f:C", lopts, NULL);

    if (c == -1)
      break;

    switch (c) {
    case 'D':
      device = strdup (optarg);
      break;
    case 's':
      speed = atoi(optarg);
      break;
    case 'd':
      delay = atoi(optarg);
      break;
    case 'r':
      reg = atoi(optarg);
      break;
    case 'v':
      val = atoi(optarg);
      break;
    case 'p':
      sscanf (optarg, "%d,%d", &x, &y);
      break;

    case 'a':
      sscanf (optarg, "%x", &addr);
      break;
    case 'T':
      sscanf (optarg, "%d,%d", &x, &y);
      // fallthrough
    case 't':
      text = 1;
      break;
    case 'b':
      backlight = atoi (optarg);
      break;
    case 'c':
      contrast = atoi (optarg);
      break;
    case 'C':
      cls = 1;
      break;
    default:
      print_usage(argv[0]);
      break;
    }
  }
  return optind; 
}



int main(int argc, char *argv[])
{
  int ret = 0;
  int fd;
  int nonoptions;
  char buf[0x100];
  int i;

  nonoptions = parse_opts(argc, argv);

  fd = open(device, O_RDWR);
  if (fd < 0)
    pabort("can't open device");

  // .xx.
  addr = addr >> 1;
  if (ioctl(fd, I2C_SLAVE, addr) < 0) 
    pabort ("cant set slave addr");

  if (cls) set_reg_value8 (fd, 0x10, 0xaa);

  if (contrast != -1) set_reg_value8 (fd, 0x12, contrast);

  if (backlight != -1) set_reg_value8 (fd, 0x13, backlight);

  if ((x != -1) && (y != -1)) {
    set_reg_value8 (fd, 0x11, (y << 5) | x);
  }

  if (reg != -1) 
    set_reg_value8 (fd, reg, val); 

  //printf ("text = %d, nonoptions = %d.\n", text, nonoptions);
  if (text) {
    buf [0] = 0;
    for (i=nonoptions; i < argc;i++) {
      if (i != nonoptions) strcat (buf, " ");
      strcat (buf, argv[i]);
    }
    send_text (fd, buf);
  }

  close(fd);

  return ret;
}
