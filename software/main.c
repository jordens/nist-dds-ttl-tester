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

	DDS_GPIO = n2;
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

	DDS_GPIO = n2;
	DDS_REG(0x0a) = ftw2 & 0xff;
	DDS_REG(0x0b) = (ftw2 >> 8) & 0xff;
	DDS_REG(0x0c) = (ftw2 >> 16) & 0xff;
	DDS_REG(0x0d) = (ftw2 >> 24) & 0xff;
	DDS_FUD = 0;
}

static void ddsreset(void)
{
	DDS_GPIO |= 1 << 7;
	DDS_FUD = 0;
	DDS_GPIO &= ~(1 << 7);
	DDS_FUD = 0;
}

static void ddstest(void)
{
	int i, j;
	unsigned int v[12] = {
		0xaaaaaaaa, 0x55555555, 0xa5a5a5a5, 0x5a5a5a5a,
		0x00000000, 0xffffffff, 0x12345678, 0x87654321,
		0x0000ffff, 0xffff0000, 0x00ff00ff, 0xff00ff00,
		};
	unsigned int f, g;

	for(i=0; i<8; i++) {
		DDS_GPIO = i;
		ddsreset();

		for (j=0; j<8; j++) {
			f = v[j];
			DDS_REG(0x0a) = f & 0xff;
			DDS_REG(0x0b) = (f >> 8) & 0xff;
			DDS_REG(0x0c) = (f >> 16) & 0xff;
			DDS_REG(0x0d) = (f >> 24) & 0xff;
			DDS_FUD = 0;
			g = DDS_REG(0x0a);
			g |= DDS_REG(0x0b) << 8;
			g |= DDS_REG(0x0c) << 16;
			g |= DDS_REG(0x0d) << 24;
			if(g != f) {
				printf("readback fail on %02x, %08x != %08x\n", i, g, f);
			}
		}
	}
}

static void help(void)
{
	puts("NIST DDS/TTL Tester");
	puts("Available commands:");
	puts("help           - this message");
	puts("revision       - display revision");
	puts("inputs         - read inputs");
	puts("ttlout <n>     - output ttl");
	puts("ttlin          - read ttll");
	puts("ddssel <n>     - select a dds");
	puts("ddsreset       - reset all dds");
	puts("ddsw <a> <d>   - write to dds register");
	puts("ddsr <a>       - read dds register");
	puts("ddsfud         - pulse FUD");
	puts("ddsftw <n> <d> - write FTW");
	puts("ddstest        - perform test sequence on dds");
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

	if(strcmp(token, "help") == 0) help();
	else if(strcmp(token, "revision") == 0) printf("%08x\n", MSC_GIT_ID);

	else if(strcmp(token, "inputs") == 0) inputs();
	else if(strcmp(token, "ttlout") == 0) ttlout(get_token(&c));
	else if(strcmp(token, "ttlin") == 0) ttlin();

	else if(strcmp(token, "ddssel") == 0) ddssel(get_token(&c));
	else if(strcmp(token, "ddsw") == 0) ddsw(get_token(&c), get_token(&c));
	else if(strcmp(token, "ddsr") == 0) ddsr(get_token(&c));
	else if(strcmp(token, "ddsreset") == 0) ddsreset();
	else if(strcmp(token, "ddsfud") == 0) ddsfud();
	else if(strcmp(token, "ddsftw") == 0) ddsftw(get_token(&c), get_token(&c));
	else if(strcmp(token, "ddstest") == 0) ddstest();

	else if(strcmp(token, "") != 0)
		printf("Command not found\n");
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
