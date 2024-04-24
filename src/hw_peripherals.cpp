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
	ResetMCU();

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
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
	bcm2835_spi_set_speed_hz(12500000); // on RPi4 this set fclock to 25 MHz
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

	// Set RESET_MCU to be an input without pullup
	bcm2835_gpio_fsel(RESET_MCU, BCM2835_GPIO_FSEL_INPT);

	// Set TP2 to be an output and preset it LOW
	bcm2835_gpio_fsel(TP2, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(TP2, LOW);
	// Set TP2 to be an output and preset it LOW
	bcm2835_gpio_fsel(TP3, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_write(TP3, LOW);

	// Set the pin to be an output
	bcm2835_gpio_fsel(SDATA, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(SCLK, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(CSn1, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(CSn2, BCM2835_GPIO_FSEL_OUTP);

	// Set the pins to low
	bcm2835_gpio_write(SDATA, LOW);
	bcm2835_gpio_write(SCLK, LOW);
	bcm2835_gpio_write(CSn1, HIGH);
	bcm2835_gpio_write(CSn2, HIGH);

	printf("IO setup done\n");
}

void ResetMCU()
{
	// Reset the MCU by setting RESET_MCU as output and LOW
	bcm2835_gpio_fsel(RESET_MCU, BCM2835_GPIO_FSEL_OUTP);

	bcm2835_gpio_write(RESET_MCU, LOW);
	bcm2835_delay(100);

	// Set RESET_MCU as input
	bcm2835_gpio_fsel(RESET_MCU, BCM2835_GPIO_FSEL_INPT);
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

/*
 * Function to send a 16-bit data with simulated SPI
 */
int my_spi_transfer(uint16_t data)
{
	uint16_t result = 0;

	for (int i = 0; i < 16; i++)
	{
		// Lower the clock
		bcm2835_gpio_write(SCLK, LOW);

		// Set the data bit
		if (data & 0x8000)
		{
			bcm2835_gpio_write(SDATA, HIGH);
		}
		else
		{
			bcm2835_gpio_write(SDATA, LOW);
		}
		// bcm2835_delayMicroseconds(30);

		// Raise the clock
		bcm2835_gpio_write(SCLK, HIGH);
		// bcm2835_delayMicroseconds(30);

		// Shift the data
		data <<= 1;
	}

	return result;
}

int Write_MCP4822(int ch, uint16_t data)
{
	uint16_t config;

	// Select the channel
	if (ch == 1)
	{
		config = CH_A;
	}
	else
	{
		config = CH_B;
	}

	config <<= 12;
	data |= config;

	// Send the data
	my_spi_transfer(data);

	return 0;
}

int WriteData(int adc, int ch, uint16_t data)
{
	// Select the adc
	if (adc == 1)
	{
		bcm2835_gpio_write(CSn1, LOW);
	}
	else if (adc == 2)
	{
		bcm2835_gpio_write(CSn2, LOW);
	}
	else
	{
		return -1;
	}
	// bcm2835_delayMicroseconds(30);

	// Write the data
	Write_MCP4822(ch, data);

	// Deselect the adc
	if (adc == 1)
	{
		bcm2835_gpio_write(CSn1, HIGH);
	}
	else if (adc == 2)
	{
		bcm2835_gpio_write(CSn2, HIGH);
	}

	return 0;
}

int Set_VG(double val, int ch)
{
	uint16_t data = mapVtoDAC(val);

	// Write the data
	if (WriteData(ch, 2, data) == 0)
	{
		D printf("VG%d set to %0.2f\n", ch, val);
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
	uint16_t data = mapVtoDAC(val * 0.5); // uA to V

	// Write the data
	if (WriteData(ch, 1, data) == 0)
	{
		D printf("Vsetpoint%d set to %0.2f\n", ch, val);
		return 0;
	}
	else
	{
		D printf("Error setting Vsetpoint%d\n", ch);
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