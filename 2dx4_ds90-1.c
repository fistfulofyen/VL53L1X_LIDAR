//Tianze Zhang,400208135, zhant22, 
//with 5 as the last digit 
//Assigned Bus Speed:30MHz (changed in Systick and PLL.h)
//with 3 as the second last digit 
//distance Statues : PF4 (implment in onboardLEDs.c, call FlashLED3(1) here to call the function )


#include <stdint.h>
#include <math.h>
#include "tm4c1294ncpdt.h"
#include "vl53l1x_api.h"
#include "PLL.h"
#include "SysTick.h"
#include "uart.h"
#include "onboardLEDs.h"

// The VL53L1X uses a slightly different way to define the default address of 0x29
// The I2C protocol defintion states that a 7-bit address is used for the device
// The 7-bit address is stored in bit 7:1 of the address register.  Bit 0 is a binary
// value that indicates if a write or read is to occur.  The manufacturer lists the 
// default address as 0x52 (0101 0010).  This is 0x29 (010 1001) with the read/write bit
// alread set to 0.
//uint16_t	dev = 0x29;

uint16_t	dev=0x52;
int status=0;
volatile int IntCount;

//device in interrupt mode (GPIO1 pin signal)
#define isInterrupt 1 /* If isInterrupt = 1 then device working in interrupt mode, else device working in polling mode */

void I2C_Init(void);
void UART_Init(void);
void PortG_Init(void);
void PortE_Init(void);
void PortF_Init(void);
void PortK_Init(void);
void VL53L1X_XSHUT(void);
void motorRun(void);

//capture values from VL53L1X for inspection
uint16_t debugArray[100];

int main(void) {
  uint8_t byteData, sensorState=0, myByteArray[10] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} , i=0;
  uint16_t wordData;
  uint8_t ToFSensor = 1; // 0=Left, 1=Center(default), 2=Right
  uint16_t Distance;
  uint16_t SignalRate;
  uint16_t AmbientRate;
  uint16_t SpadNum; 
  uint8_t RangeStatus;
  uint8_t dataReady;
	float x;
	float y;
	float z=0.00;
	float deg;

	//initialize
	PLL_Init();	
	SysTick_Init();
	onboardLEDs_Init();
	I2C_Init();
	UART_Init();
	PortE_Init(); //button	PEO
	PortF_Init();	//onboard led PF4 for the state changing flag. 
	PortK_Init();	//motor PK0 to PK3
	
/* Those basic I2C read functions can be used to check your own I2C functions */
  status = VL53L1_RdByte(dev, 0x010F, &byteData);					// This is the model ID.  Expected returned value is 0xEA
  myByteArray[i++] = byteData;

  status = VL53L1_RdByte(dev, 0x0110, &byteData);					// This is the module type.  Expected returned value is 0xCC
  myByteArray[i++] = byteData;
	
	status = VL53L1_RdWord(dev, 0x010F, &wordData);
	status = VL53L1X_GetSensorId(dev, &wordData);

	// Booting ToF chip
	while(sensorState==0){
		status = VL53L1X_BootState(dev, &sensorState);
		SysTick_Wait10ms(10);
  }
	FlashAllLEDs();

	status = VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/
  /* This function must to be called to initialize the sensor with the default setting  */
  status = VL53L1X_SensorInit(dev);
  status = VL53L1X_StartRanging(dev);   /* This function is called to enable the ranging */

	uint8_t round=0;
	while(1){
	
	if((GPIO_PORTE_DATA_R&0b00000001)==0b00000000){ //button PEO is pressed
		
		for(uint32_t i=0;i<=512;i++){	//one cycle which is 512 steps. 
			if (i!=0&&i%3==0){					//check for 3 steps the special angle I specified. 
				status = VL53L1X_GetDistance(dev, &Distance);
				FlashLED3(1); //state changing signal.  which is PF4 arrocidng to my student number. 
				
				deg=(float)i/512*2*3.141592653; //here we convert the deg to a radian number in order to do the below X and Y calculation. 
				
				x=Distance*sin(deg); //we calcualte X and Y from Pythagorean Theorem
				y=Distance*cos(deg);

			 status = VL53L1X_ClearInterrupt(dev); /* clear interrupt has to be called to enable next interrupt*/
			 sprintf(printf_buffer,"%f %f %f\r\n", x, y, z);			 //print out X Y Z data using UART and tranfer to PC 
			 UART_printf(printf_buffer);
			 sprintf(printf_buffer,"%f %f %f\r\n", x, y, z+200.00);
			 UART_printf(printf_buffer);
			 sprintf(printf_buffer,"%f %f %f\r\n", x, y, z+400.00);
			 UART_printf(printf_buffer);
			 sprintf(printf_buffer,"%f %f %f\r\n", x, y, z+800.00);
			 UART_printf(printf_buffer);
			}
			motorRun();		//motor run for one step
		}
	}
}
VL53L1X_StopRanging(dev);
  while(1) {}
}

#define I2C_MCS_ACK             0x00000008  // Data Acknowledge Enable
#define I2C_MCS_DATACK          0x00000008  // Acknowledge Data
#define I2C_MCS_ADRACK          0x00000004  // Acknowledge Address
#define I2C_MCS_STOP            0x00000004  // Generate STOP
#define I2C_MCS_START           0x00000002  // Generate START
#define I2C_MCS_ERROR           0x00000002  // Error
#define I2C_MCS_RUN             0x00000001  // I2C Master Enable
#define I2C_MCS_BUSY            0x00000001  // I2C Busy
#define I2C_MCR_MFE             0x00000010  // I2C Master Function Enable
#define MAXRETRIES              5           // number of receive attempts before giving up

void I2C_Init(void){
  SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R0;           // activate I2C0
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;          // activate port B
  while((SYSCTL_PRGPIO_R&0x0002) == 0){};// ready?

    GPIO_PORTB_AFSEL_R |= 0x0C;           // 3) enable alt funct on PB2,3       0b00001100
    GPIO_PORTB_ODR_R |= 0x08;             // 4) enable open drain on PB3 only

    GPIO_PORTB_DEN_R |= 0x0C;             // 5) enable digital I/O on PB2,3
//    GPIO_PORTB_AMSEL_R &= ~0x0C;          // 7) disable analog functionality on PB2,3

                                                                                // 6) configure PB2,3 as I2C
//  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00003300;
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0xFFFF00FF)+0x00002200;    //TED
    I2C0_MCR_R = I2C_MCR_MFE;                      // 9) master function enable
    I2C0_MTPR_R = 0b0000000000000101000000000111011;                                        // 8) configure for 100 kbps clock (added 8 clocks of glitch suppression ~50ns)
}




/********************Initialize For Pins************************/

void PortE_Init(void){			 //button input PE0
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4;
		while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R4) == 0){};
		GPIO_PORTE_DEN_R = 0b00000001; 
		GPIO_PORTE_DIR_R =0b00000000;
	return;
}

void PortK_Init(void){ 			//motor PK0-PK3
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R9;
		while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R9) == 0){};
		GPIO_PORTK_DEN_R = 0b00001111; 
		GPIO_PORTK_DIR_R =0b00001111; 
	return;
}


void PortF_Init(void){         //onboard LED PF4
  SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;                
	while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R5) == 0){};
	GPIO_PORTF_DIR_R=0b00010000;
	GPIO_PORTF_DEN_R=0b00010000;
	return;
}
void PortG_Init(void){
    //Use PortG0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R6;                // activate clock for Port N
    while((SYSCTL_PRGPIO_R&SYSCTL_PRGPIO_R6) == 0){};    // allow time for clock to stabilize
    GPIO_PORTG_DIR_R &= 0x00;                                        // make PG0 in (HiZ)
  GPIO_PORTG_AFSEL_R &= ~0x01;                                     // disable alt funct on PG0
  GPIO_PORTG_DEN_R |= 0x01;                                        // enable digital I/O on PG0
                                                                                                    // configure PG0 as GPIO
  //GPIO_PORTN_PCTL_R = (GPIO_PORTN_PCTL_R&0xFFFFFF00)+0x00000000;
  GPIO_PORTG_AMSEL_R &= ~0x01;                                     // disable analog functionality on PN0
  return;
}
/***********************Run Motor By One Cycle**********************************/
void motorRun(void){
	GPIO_PORTK_DATA_R = 0b00001100;
	// toggle LED for visualization ofprocess
	SysTick_Wait10ms(1);
	// wait 10ms (assumes 120 MHz clock)
	GPIO_PORTK_DATA_R = 0b00000110;
	// toggle LED for visualization ofprocess
	SysTick_Wait10ms(1);
	GPIO_PORTK_DATA_R = 0b00000011;
	// toggle LED for visualization ofprocess
	SysTick_Wait10ms(1);
	GPIO_PORTK_DATA_R = 0b00001001;
	// toggle LED for visualization ofprocess
	SysTick_Wait10ms(1);
}

//XSHUT     This pin is an active-low shutdown input; the board pulls it up to VDD to enable the sensor by default. Driving this pin low puts the sensor into hardware standby. This input is not level-shifted.
void VL53L1X_XSHUT(void){
    GPIO_PORTG_DIR_R |= 0x01;                                        // make PG0 out
    GPIO_PORTG_DATA_R &= 0b11111110;                                 //PG0 = 0
    FlashAllLEDs();
    SysTick_Wait10ms(10);
    GPIO_PORTG_DIR_R &= ~0x01;                                            // make PG0 input (HiZ)
}