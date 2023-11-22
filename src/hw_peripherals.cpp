#include "hw_peripherals.hpp"

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>

int uart0_filestream;		   // file descriptor for the UART
int last_response_status = -1; // last response status from the PIC
struct cmd last_cmd;		   // last command sent to the PIC
struct termios termAttr;	   // terminal attributes
struct sigaction saio;		   // signal action

int init_system()
{
	InitBCM2835();
	SetupSPI();
	SetupIO();
	SetupOpenConfigUSART0();
	runFirmware(0x08);
	Set_T2lock(1);
	usleep(300000);
	Set_AcqEnabled(1);
	usleep(300000);
	// InitADC1();
	// usleep(300000);
	// InitADC2();
	// usleep(300000);

	return 0;
}

void InitBCM2835()
{
	if (!bcm2835_init())
	{
		printf("bcm2835_init failed. Are you running as root??\n");
		return;
	}
	else
	{
		printf("bcm2835_init OK\n");
	}
}

void SetupSPI()
{
	bcm2835_spi_begin();
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE1);
	bcm2835_spi_setClockDivider(MY_SPI_SCLK_DIVIDER);
	bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
	bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

	printf("SPI setup done\n");
}

void SetupIO()
{
	// Set REQ to be an output and preset it HIGH
	bcm2835_gpio_fsel(REQ, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(REQ, HIGH);

	// Set ACK0 to be an input with a pullup
	bcm2835_gpio_fsel(ACK0, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(ACK0, BCM2835_GPIO_PUD_UP);
	// Set ACK1 to be an input with a pullup
	bcm2835_gpio_fsel(ACK1, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(ACK1, BCM2835_GPIO_PUD_UP);
	// Set ACK2 to be an input with a pullup
	bcm2835_gpio_fsel(ACK2, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(ACK2, BCM2835_GPIO_PUD_UP);

	// Set DSPIC_STATUS to be an input with a pulldown
	bcm2835_gpio_fsel(DSPIC_STATUS, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(DSPIC_STATUS, BCM2835_GPIO_PUD_DOWN);
	// Set PIC_DATA to be an input with a pulldown
	bcm2835_gpio_fsel(PIC_DATA, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(PIC_DATA, BCM2835_GPIO_PUD_DOWN);
	// Set CZMQ_STATUS to be an output and preset it LOW
	bcm2835_gpio_fsel(CZMQ_STATUS, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(CZMQ_STATUS, LOW);

	// Set TP1 to be an output and preset it LOW
	bcm2835_gpio_fsel(TP1, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(TP1, LOW);
	// Set TP2 to be an output and preset it LOW
	bcm2835_gpio_fsel(TP2, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(TP2, LOW);
	// Set TP2 to be an output and preset it LOW
	bcm2835_gpio_fsel(TP3, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(TP3, LOW);

	printf("IO setup done\n");
}

void signal_handler_UART(int status)
{
	char buf[256];

	int n = read(uart0_filestream, &buf, sizeof(buf));

	if (n > 0)
	{
		D printf("Received %d bytes from UART\n", n);
		if (buf[0] == last_cmd.id)
			last_response_status = 0;
	}
	else
	{
		D printf("Error reading from UART\n");
		last_response_status = -1;
	}
}

void SetupOpenConfigUSART0()
{
	struct termios options;

	uart0_filestream = -1;
	uart0_filestream = open("/dev/serial0", O_RDWR | O_NOCTTY);
	if (uart0_filestream == -1)
	{
		// ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART. Ensure it is not in use by another application\n");
	}

	saio.sa_handler = signal_handler_UART;
	saio.sa_flags = 0;
	saio.sa_restorer = NULL;
	sigaction(SIGIO, &saio, NULL);

	fcntl(uart0_filestream, F_SETFL, FNDELAY);
	fcntl(uart0_filestream, F_SETOWN, getpid());
	fcntl(uart0_filestream, F_SETFL, O_ASYNC);

	tcgetattr(uart0_filestream, &options);
	options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);

	printf("UART setup done\n");
}

void CloseUSART0()
{
	close(uart0_filestream);
}

void TxByte(char c)
{
	if (uart0_filestream != -1)
	{
		int count = write(uart0_filestream, &c, 1);
		if (count < 0)
		{
			printf("UART TX error\n");
		}
	}
}

int sendCommandTodsPic(struct cmd command)
{
	struct timeval sta, sto;
	long s, us, ms;

	last_response_status = -1;
	last_cmd = command;

	tcflush(uart0_filestream, TCIOFLUSH);
	TxByte(command.id);

	D printf("Sent command %c to dsPIC with parameters: ", command.id);
	for (int i = 0; i < command.numbytepars; i++)
		TxByte(command.bytePars[i]);

	D
	{
		for (int i = 0; i < command.numbytepars; i++)
		{
			printf("%d ", command.bytePars[i]);
		}
		printf("\n");
	}

	// Wait for the response from the PIC for a maximum of TIMEOUT_RXBACK_CMD ms
	gettimeofday(&sta, NULL);
	ms = 0;
	while ((last_response_status == -1) && (ms < TIMEOUT_RXBACK_CMD))
	{
		gettimeofday(&sto, NULL);
		s = sto.tv_sec - sta.tv_sec;
		us = sto.tv_usec - sta.tv_usec;
		ms = (s * 1000.0 + us / 1000.0);
	}

	return last_response_status;
}

void runFirmware(unsigned char val)
{
	tcflush(uart0_filestream, TCIFLUSH);
	TxByte(val);

	uint8_t dspic_is_ready;

	printf("Waiting for firmware to run on dsPIC...\n");
	do
	{
		dspic_is_ready = bcm2835_gpio_lev(DSPIC_STATUS);
	} while (dspic_is_ready == 0);

	printf("Firmware running on dsPIC\n");
}

void Set_T2lock(unsigned char val)
{
	struct cmd command;
	command.id = CMD_SET_T2LOCK;
	command.pars[0] = val;
	command.numbytepars = 1;
	command.bytePars[0] = val;

	if (sendCommandTodsPic(command) == 0)
	{
		D printf("T2 lock set to %d\n", val);
	}
	else
	{
		D printf("Error setting T2 lock\n");
	}
}

void Set_AcqEnabled(unsigned char val)
{
	struct cmd command;
	command.id = CMD_SET_ACQENABLED;
	command.pars[0] = val;
	command.numbytepars = 1;
	command.bytePars[0] = val;

	if (sendCommandTodsPic(command) == 0)
	{
		D printf("Acquisition enabled\n");
	}
	else
	{
		D printf("Error enabling acquisition\n");
	}
}

void InitADC1()
{
	struct cmd command;
	command.id = CMD_INIT_ADC1;
	command.pars[0] = 0;
	command.numbytepars = 0;
	command.bytePars[0] = 0;

	if (sendCommandTodsPic(command) == 0)
	{
		D printf("ADC1 initialized\n");
	}
	else
	{
		D printf("Error initializing ADC1\n");
	}
}

void InitADC2()
{
	struct cmd command;
	command.id = CMD_INIT_ADC2;
	command.pars[0] = 0;
	command.numbytepars = 0;
	command.bytePars[0] = 0;

	if (sendCommandTodsPic(command) == 0)
	{
		D printf("ADC2 initialized\n");
	}
	else
	{
		D printf("Error initializing ADC2\n");
	}
}

void ResetDSPIC()
{
	struct cmd command;
	command.id = CMD_RESET;
	command.pars[0] = 0;
	command.numbytepars = 0;
	command.bytePars[0] = 0;

	if (sendCommandTodsPic(command) == 0)
	{
		D printf("dsPIC reset\n");
	}
	else
	{
		D printf("Error resetting dsPIC\n");
	}
}

int Set_VG(double val, int ch)
{
	struct cmd command;
	if (ch == 1)
		command.id = CMD_SET_VSOURCE1;
	else if (ch == 2)
		command.id = CMD_SET_VSOURCE2;
	else
		return -1;
	command.pars[0] = val * 1000.0; // convert to mV
	command.numbytepars = 2;

	if (command.pars[0] < DAC_VMIN)
		command.pars[0] = DAC_VMIN;
	else if (command.pars[0] > DAC_VMAX)
		command.pars[0] = DAC_VMAX;

	unsigned short upar = (unsigned short)command.pars[0];

	command.bytePars[0] = (unsigned char)((upar & 0xFF00) >> 8);
	command.bytePars[1] = (unsigned char)(upar & 0x00FF);

	if (sendCommandTodsPic(command) == 0)
	{
		D printf("VG%d set to %f V\n", ch, val);
		return 0;
	}
	else
	{
		D printf("Error setting VG%d\n", ch);
		return -1;
	}
}

int Set_Vsetpoint(double val, int ch)
{
	struct cmd command;
	if (ch == 1)
		command.id = CMD_SET_VGATE1;
	else if (ch == 2)
		command.id = CMD_SET_VGATE2;
	else
		return -1;
	command.pars[0] = val * 500.0; // convert I (uA) to mV
	command.numbytepars = 2;

	if (command.pars[0] < DAC_VMIN)
		command.pars[0] = DAC_VMIN;
	else if (command.pars[0] > DAC_VMAX)
		command.pars[0] = DAC_VMAX;

	unsigned short upar = (unsigned short)command.pars[0];

	command.bytePars[0] = (unsigned char)((upar & 0xFF00) >> 8);
	command.bytePars[1] = (unsigned char)(upar & 0x00FF);

	if (sendCommandTodsPic(command) == 0)
	{
		D printf("Ids_setpoint%d set to %f \u03BCA\n", ch, val);
		return 0;
	}
	else
	{
		D printf("Error setting Ids_setpoint%d\n", ch);
		return -1;
	}
}

int Set_T2(double us)
{
	struct cmd command;
	command.id = CMD_SET_TIM2PER;
	command.pars[0] = DSPIC_CLOCK_MHz * us;
	command.numbytepars = 2;

	unsigned short upar = (unsigned short)command.pars[0];
	command.bytePars[0] = (unsigned char)((upar & 0xFF00) >> 8);
	command.bytePars[1] = (unsigned char)(upar & 0x00FF);

	if (sendCommandTodsPic(command) == 0)
	{
		D printf("T2 set to %0.2f\n", us);
		return 0;
	}
	else
	{
		D printf("Error setting T2\n");
		return -1;
	}
}