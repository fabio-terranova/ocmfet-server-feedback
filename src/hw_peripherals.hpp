#pragma once

#include <bcm2835.h>

// #define DEBUG
#ifdef DEBUG
#define D if (1)
#else
#define D if (0)
#endif

// #define NO_FEEDBACK 1

#define T2_DEFAULT 44

// Command-related defines
#define CMD_SET_VGATE1     'A'
#define CMD_SET_VSOURCE1   'B'
#define CMD_SET_VGATE2     'C'
#define CMD_SET_VSOURCE2   'D'
#define CMD_SET_TIM2PER    '2'
#define CMD_SET_MODE       'M'
#define CMD_SET_ACQENABLED 'N'
#define CMD_SET_T2LOCK     'P'
#define CMD_INIT_ADC1      'U'
#define CMD_INIT_ADC2      'V'
#define CMD_RESET          'W'
#define CMD_ASK_DATA       'T'
#define CMD_REBOOT         'R'
#define CMD_SET_SPISEND    'a'
#define CMD_SET_SPITX      'b'
#define CMD_SET_DATACONV   'c'
#define CMD_RESET_SPIIDX   'd'
#define CMD_QUIT_DBG       'q'
#define CMD_SAVE_PARAM     's'

// GPIO-related defines
#define REQ      RPI_V2_GPIO_P1_22 // REQ (Rpi output) is GPIO25, pin P1-22
#define ACK0     RPI_V2_GPIO_P1_18 // ACK0 (Rpi input) is GPIO24, pin P1-18
#define ACK1     RPI_V2_GPIO_P1_12 // ACK1 (Rpi input) is GPIO18, Pin P1-12
#define ACK2     RPI_V2_GPIO_P1_16 // ACK2 (Rpi input) is GPIO23, Pin P1-16
#define PIC_DATA RPI_V2_GPIO_P1_07 // PIC_DATA (Rpi input) is GPIO04, pin P1-07
#define RESET_MCU                                                              \
  RPI_V2_GPIO_P1_36           // MCU (Rpi input and output) is GPIO16, pin P1-36
#define TP2 RPI_V2_GPIO_P1_32 // TP2 (Rpi output) is GPIO12, pin P1-32
#define TP3 RPI_V2_GPIO_P1_40 // TP3 (Rpi output) is GPIO21, pin P1-40
// DAC-related defines
#define SDATA RPI_V2_GPIO_P1_32 // SDATA (Rpi output) is pin P1-32, GPIO12
#define SCLK  RPI_V2_GPIO_P1_26 // SCLK  (Rpi output) is pin P1-32, GPIO12
#define CSn1  RPI_V2_GPIO_P1_24 // CSn1  (Rpi output) is pin P1-24, GPIO8
#define CSn2  RPI_V2_GPIO_P1_16 // CSn2  (Rpi output) is pin P1-16, GPIO23

// SPI-related defines
#define MY_SPI_SCLK_DIVIDER 50
#define NO_BUFFER           -1
#define BUFFER_A            0
#define BUFFER_B            1
#ifdef NO_FEEDBACK
#define BUF_LEN 36
#else
#define BUF_LEN 32
#endif

// DAC-related defines
#define V_REF  2.048 // Reference voltage
#define GAIN2x 1
#if GAIN2x
#define G    2
#define CH_A 0x1 // 0001
#define CH_B 0x9 // 1001
#else
#define G    1
#define CH_A 0x3 // 0011
#define CH_B 0xB // 1011
#endif
#define mapVtoDAC(x) (uint16_t)((x) * 4096 / (G * V_REF))

// ADC-related defines
#define ADC_VMIN_V -5.0 // Volt
#define ADC_VMAX_V 5.0  // Volt
#define mapRAWADCtoV(x)                                                        \
  (double)((x) * (ADC_VMAX_V - ADC_VMIN_V) / 65536.0 + ADC_VMIN_V)
#define mapADCVto_uA(x) ((x) * 0.2)

// USART-related defines
#define MAXPARS            5
#define MAXBYTEPARS        10
#define TIMEOUT_RXBACK_CMD 300 // ms

// dsPIC commands structure
#define DSPIC_CLOCK_MHz 60.0
struct cmd {
  unsigned char id;                    // command letter, e.g. 'A'
  unsigned char numbytepars;           // number of byte parameters
  double        pars[MAXPARS];         // parameters
  unsigned char bytePars[MAXBYTEPARS]; // byte parameters
};

int  init_system();
void initBCM2835(); // initialize the BCM2835 library
void setupSPI();    // setup the SPI
void setupIO();     // setup the IO
void resetMCU();    // reset the MCU
void signal_handler_UART(int);
void setupOpenConfigUSART0(); // setup the USART
void closeUSART0();
void txByte(char);                   // send a byte to the PIC via the UART
int  sendCommandTodsPic(struct cmd); // send a command to the dsPIC
void set_T2lock(unsigned char);      // set the T2 lock
int  my_spi_transfer(uint16_t);      // transfer data via simulated SPI
int  write_MCP4882(int, uint16_t);   // write to the MPC4882
int  writeData(int, int, uint16_t);  // write data to the two ADCs
int  set_VG(double, int); // set the VG voltage for the specified channel
int  set_Vsetpoint(double,
                   int); // set the Vsetpoint voltage for the specified channel
int  set_T2(double);
