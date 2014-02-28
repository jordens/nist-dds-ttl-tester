#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <irq.h>
#include <uart.h>
#include <generated/csr.h>
#include <console.h>

static void inputs(void)
{
	int inp;

	inp = test_inputs_in_read();
	printf("PMT0: %d\n", !!(inp & 0x01));
	printf("PMT1: %d\n", !!(inp & 0x02));
	printf("PMT2: %d\n", !!(inp & 0x04));
	printf("XTRIG:%d\n", !!(inp & 0x08));
}

static void ttlout(char *value)
{
	char *c;
	unsigned int value2;

	if(*value == 0) {
		printf("ttlout <value>\n");
		return;
	}

	value2 = strtoul(value, &c, 0);
	if(*c != 0) {
		printf("incorrect value\n");
		return;
	}

	test_ttl_oe_write(0x3);
	test_ttl_o_write(value2);
}

static void ttlin(void)
{
	test_ttl_oe_write(0x0);
	printf("0x%04x\n", test_ttl_i_read());
}

#define DDS_REG(x)		MMPTR(0xb0000000 + 4*(x))
#define DDS_FUD			MMPTR(0xb0000100)
#define DDS_GPIO		MMPTR(0xb0000104)

static void ddssel(char *n)
{
	char *c;
	unsigned int n2;

	if(*n == 0) {
		printf("ddssel <n>\n");
		return;
	}

	n2 = strtoul(n, &c, 0);
	if(*c != 0) {
		printf("incorrect number\n");
		return;
	}

	DDS_GPIO = 1 << n2;
}

static void ddsw(char *addr, char *value)
{
	char *c;
	unsigned int addr2, value2;

	if((*addr == 0) || (*value == 0)) {
		printf("ddsr <addr> <value>\n");
		return;
	}

	addr2 = strtoul(addr, &c, 0);
	if(*c != 0) {
		printf("incorrect address\n");
		return;
	}
	value2 = strtoul(value, &c, 0);
	if(*c != 0) {
		printf("incorrect value\n");
		return;
	}

	DDS_REG(addr2) = value2;
}

static void ddsr(char *addr)
{
	char *c;
	unsigned int addr2;

	if(*addr == 0) {
		printf("ddsr <addr>\n");
		return;
	}

	addr2 = strtoul(addr, &c, 0);
	if(*c != 0) {
		printf("incorrect address\n");
		return;
	}

	printf("0x%02x\n", DDS_REG(addr2));
}

static void ddsfud(void)
{
	DDS_FUD = 0;
}

static void ddsftw(char *n, char *ftw)
{
	char *c;
	unsigned int n2, ftw2;

	if((*n == 0) || (*ftw == 0)) {
		printf("ddsftw <n> <ftw>\n");
		return;
	}

	n2 = strtoul(n, &c, 0);
	if(*c != 0) {
		printf("incorrect number\n");
		return;
	}
	ftw2 = strtoul(ftw, &c, 0);
	if(*c != 0) {
		printf("incorrect value\n");
		return;
	}

	DDS_GPIO = 1 << n2;
	DDS_REG(0x0a) = ftw2 & 0xff;
	DDS_REG(0x0b) = (ftw2 >> 8) & 0xff;
	DDS_REG(0x0c) = (ftw2 >> 16) & 0xff;
	DDS_REG(0x0d) = (ftw2 >> 24) & 0xff;
	DDS_FUD = 0;
}

static void readstr(char *s, int size)
{
	char c[2];
	int ptr;

	c[1] = 0;
	ptr = 0;
	while(1) {
		c[0] = readchar();
		switch(c[0]) {
			case 0x7f:
			case 0x08:
				if(ptr > 0) {
					ptr--;
					putsnonl("\x08 \x08");
				}
				break;
			case 0x07:
				break;
			case '\r':
			case '\n':
				s[ptr] = 0x00;
				putsnonl("\n");
				return;
			default:
				putsnonl(c);
				s[ptr] = c[0];
				ptr++;
				break;
		}
	}
}

static char *get_token(char **str)
{
	char *c, *d;

	c = (char *)strchr(*str, ' ');
	if(c == NULL) {
		d = *str;
		*str = *str+strlen(*str);
		return d;
	}
	*c = 0;
	d = *str;
	*str = c+1;
	return d;
}

static void do_command(char *c)
{
	char *token;

	token = get_token(&c);

	if(strcmp(token, "inputs") == 0) inputs();
	else if(strcmp(token, "ttlout") == 0) ttlout(get_token(&c));
	else if(strcmp(token, "ttlin") == 0) ttlin();

	else if(strcmp(token, "ddssel") == 0) ddssel(get_token(&c));
	else if(strcmp(token, "ddsw") == 0) ddsw(get_token(&c), get_token(&c));
	else if(strcmp(token, "ddsr") == 0) ddsr(get_token(&c));
	else if(strcmp(token, "ddsfud") == 0) ddsfud();
	else if(strcmp(token, "ddsftw") == 0) ddsftw(get_token(&c), get_token(&c));
	else puts("Unknown command");
}

int main(void)
{
	char buffer[64];

	irq_setmask(0);
	irq_setie(1);
	uart_init();
	
	puts("DDS/TTL test software built "__DATE__" "__TIME__"\n");
		
	while(1) {
		putsnonl("\e[1mtester>\e[0m ");
		readstr(buffer, 64);
		do_command(buffer);
	}
	
	return 0;
}
