#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h>
#include <stdlib.h>
#include <ads1115.h>
#include <byteswap.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define i2cAddr 0x48

#define	CONFIG_OS_MASK		(0x8000)	// Operational Status Register
#define	CONFIG_OS_SINGLE	(0x8000)	// Write - Starts a single-conversion
						// Read    1 = Conversion complete

// The multiplexor

#define	CONFIG_MUX_MASK		(0x7000)
	
// Single-ended modes

#define	CONFIG_MUX_SINGLE_0	(0x4000)	// AIN0
#define	CONFIG_MUX_SINGLE_1	(0x5000)	// AIN1
#define	CONFIG_MUX_SINGLE_2	(0x6000)	// AIN2
#define	CONFIG_MUX_SINGLE_3	(0x7000)	// AIN3

// Programmable Gain Amplifier

#define	CONFIG_PGA_MASK		(0x0E00)
#define	CONFIG_PGA_6_144V	(0x0000)	// +/-6.144V range = Gain 2/3
#define	CONFIG_PGA_4_096V	(0x0200)	// +/-4.096V range = Gain 1
#define	CONFIG_PGA_2_048V	(0x0400)	// +/-2.048V range = Gain 2 (default)
#define	CONFIG_PGA_1_024V	(0x0600)	// +/-1.024V range = Gain 4
#define	CONFIG_PGA_0_512V	(0x0800)	// +/-0.512V range = Gain 8
#define	CONFIG_PGA_0_256V	(0x0A00)	// +/-0.256V range = Gain 16

#define	CONFIG_MODE		(0x0100)	// 0 is continuous, 1 is single-shot (default)

// Data Rate

#define	CONFIG_DR_MASK		(0x00E0)
#define	CONFIG_DR_8SPS		(0x0000)	//   8 samples per second
#define	CONFIG_DR_16SPS		(0x0020)	//  16 samples per second
#define	CONFIG_DR_32SPS		(0x0040)	//  32 samples per second
#define	CONFIG_DR_64SPS		(0x0060)	//  64 samples per second
#define	CONFIG_DR_128SPS	(0x0080)	// 128 samples per second (default)
#define	CONFIG_DR_475SPS	(0x00A0)	// 475 samples per second
#define	CONFIG_DR_860SPS	(0x00C0)	// 860 samples per second

// Comparator mode

#define	CONFIG_CMODE_MASK	(0x0010)
#define	CONFIG_CMODE_TRAD	(0x0000)	// Traditional comparator with hysteresis (default)
#define	CONFIG_CMODE_WINDOW	(0x0010)	// Window comparator

// Comparator polarity - the polarity of the output alert/rdy pin

#define	CONFIG_CPOL_MASK	(0x0008)
#define	CONFIG_CPOL_ACTVLOW	(0x0000)	// Active low (default)
#define	CONFIG_CPOL_ACTVHI	(0x0008)	// Active high

// Latching comparator - does the alert/rdy pin latch

#define	CONFIG_CLAT_MASK	(0x0004)
#define	CONFIG_CLAT_NONLAT	(0x0000)	// Non-latching comparator (default)
#define	CONFIG_CLAT_LATCH	(0x0004)	// Latching comparator

#define	CONFIG_DEFAULT		(0x8583)	// From the datasheet

int pinBase = 65;
int medir(int pino, struct wiringPiNodeStruct *node )
{
	//ads1115Setup
	//struct wiringPiNodeStruct *node ;
	
  	int fd;
	int pin = pinBase + pino;
	printf("Meu pin: %d", pin);
	//exit(0);
  	fd = wiringPiI2CSetup (i2cAddr);

  	node->fd           = fd ;
 	node->data0        = CONFIG_PGA_4_096V ;	// Gain in data0
  	node->data1        = CONFIG_DR_128SPS ;	// Samples/sec in data1
  	
	//myanalogread	
	int chan = pin - node->pinBase ;
	int16_t  result ;
	uint16_t config = CONFIG_DEFAULT ;

  	chan &= 7 ;

	// Setup the configuration register

	//	Set PGA/voltage range

  	config &= ~CONFIG_PGA_MASK ;
  	config |= node->data0 ;

	//	Set sample speed

 	config &= ~CONFIG_DR_MASK ;
  	config |= node->data1 ;

	//	Set single-ended channel or differential mode

  	config &= ~CONFIG_MUX_MASK ;

  	switch (chan)
  	{
    		case 0: config |= CONFIG_MUX_SINGLE_0 ; break ;
    		case 1: config |= CONFIG_MUX_SINGLE_1 ; break ;
    		case 2: config |= CONFIG_MUX_SINGLE_2 ; break ;
    		case 3: config |= CONFIG_MUX_SINGLE_3 ; break ;
	  }
	
	//	Start a single conversion
	
	config |= CONFIG_OS_SINGLE ;
	config = __bswap_16 (config) ;
	wiringPiI2CWriteReg16 (node->fd, 1, config) ;
	
	// Wait for the conversion to complete
	
	for (;;)
	{
	    result =  wiringPiI2CReadReg16 (node->fd, 1) ;
	    result = __bswap_16 (result) ;
	    if ((result & CONFIG_OS_MASK) != 0)
	    break ;
	    delayMicroseconds (100) ;
	}
	
	result =  wiringPiI2CReadReg16 (node->fd, 0) ;
	result = __bswap_16 (result) ;
	
	// Sometimes with a 0v input on a single-ended channel the internal 0v reference
	//	can be higher than the input, so you get a negative result...
	
	if ( (chan < 4) && (result < 0)) 
		return 0;
	else
		return result;
}

/*
double calc_gas(int sensorValue)
{
	float sensor_volt; //Definindo variavel da conversão de tensão
  	float RS_gas; //Definindo variavel de resistencia do gas
  	float ratio; //Define variable para taxa de gás
	float m = -0.318; //inclinação
	float b = 1.133; //Y-interceção
	float R0 = 11.820; //Sensor de resistencia em ar livre de metano obtido experimentalmente
	sensor_volt = sensorValue * (4.069 / 32768); 
	printf("\n sensor_volt: %f \n\n",sensor_volt);
  	RS_gas = ((5.0 * 10.0) / sensor_volt) - 10.0; //Get value of RS in a gas
  	ratio = RS_gas / R0;   // Get ratio RS_gas/RS_air

  	double ppm_log = (log10(ratio) - b) / m; //Get ppm value in linear scale according to the the ratio value
  	double ppm = pow(10, ppm_log); //Convert ppm value to log scale
  	double percentage = ppm / 10000; //Convert to percentage
	return percentage;
}
*/

int main() 
{
	wiringPiSetup();
	int pin_pressao=0,pin_gas=1, pin_ph=2;
	int resultado=0;

	struct wiringPiNodeStruct *node ;
	node = wiringPiNewNode (pinBase, 8);
	
	int sensorValue0 = medir(pin_pressao, node);
	//double pressao = calc_pressao(sensorValue0);
        //printf("\n Pressão: %f \n\n", pressao);
	printf("\n Pressão: %d \n\n", sensorValue0);
	
	int sensorValue1 = medir(pin_gas, node);
	//double percent_gas = calc_gas(sensorValue1);
  	//printf(" A porcentagem de gás metano eh = %.2f\n",percent_gas);
	printf("\n Ph: %d \n\n", sensorValue1);

	int sensorValue2 = medir(pin_ph, node);
	//double ph = calc_ph(sensorValue2);
  	//printf(" Ph: %.2f\n",ph);
	printf("\n Ph: %d \n", sensorValue2);
	return 0;
}