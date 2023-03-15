#include <Arduino.h>
#include <Wire.h>
#include <ADC.h>
#include <ADC_util.h>

#define ADXL355_RANGE_2G 		0x01
#define ADXL355_RANGE_4G 		0x02
#define ADXL355_RANGE_8G 		0x03
#define ADXL355_MEASURE_MODE 	0x00 // 0x06 can also be used; something to do with the thermometer
#define ADXL355_LPF 			0x00 // Currently set to 4000 / 1000 Hz (ODR / LPF respectively), see page 37 of datasheet //ODR = output data rate
#define ADXL355_ADDRESS 		0x53 // Alternive of 0x1D, for that one do not connect Miso/asel to 3v3.

#define ADXL355_XDATA3 			0x08
#define ADXL355_XDATA2 			0x09
#define ADXL355_XDATA1 			0x0A
#define ADXL355_YDATA3 			0x0B
#define ADXL355_YDATA2 			0x0C // these are all the registers to the data coming from the ADXL355
#define ADXL355_YDATA1 			0x0D // Per axis, expect 20 bit (3 bytes - some) of data in 2G mode, therefor 3 registers.
#define ADXL355_ZDATA3 			0x0E
#define ADXL355_ZDATA2 			0x0F
#define ADXL355_ZDATA1 			0x10

#define ADXL355_RANGE 			0x2C
#define ADXL355_POWER_CTL 		0x2D
#define ADXL355_FILTER 			0x28

#define ADXL355_ODR_RATE		250	//us --> 1/4000 Hz

#define ADXL1002_ADC_PIN		A17
#define ADXL1002_SAMPLING_RATE	1	//us --> 1 Msps ??


elapsedMillis print_timer;
elapsedMicros adxl_odr_timer_check, adxl1002_adc_timer;



uint32_t adxl355_xreading1 = 0;
uint32_t adxl355_xreading2 = 0;
uint32_t adxl355_xreading3 = 0;
uint32_t adxl355_yreading1 = 0;
uint32_t adxl355_yreading2 = 0;
uint32_t adxl355_yreading3 = 0;
uint32_t adxl355_zreading1 = 0;
uint32_t adxl355_zreading2 = 0;
uint32_t adxl355_zreading3 = 0;	

int64_t xdata = 0, ydata = 0, zdata = 0;


ADC *adc = new ADC();

int adxl1002_value = 0;

void setup()	
{

	Serial.begin(9600);          // start serial communication at 9600bps	
	Wire.begin();

	/////////////////////////////////////////////////// ADXL1002 ////////////////////////////////////////////////////

	pinMode(ADXL1002_ADC_PIN, INPUT_DISABLE);

	// ADC0 //
	adc->adc0->setAveraging(16);
	adc->adc0->setResolution(16);
	adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);
	adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED);

	/////////////////////////////////////////////////// ADXL355 /////////////////////////////////////////////////////

	/*
		Order of communication:
		1) begin transmission with device
		2) send register address
		3) send setting code (check datasheet)
	*/

	Wire.beginTransmission(ADXL355_ADDRESS);
	Wire.write(ADXL355_POWER_CTL); 
	Wire.write(ADXL355_MEASURE_MODE); //Sets the setting to measure instead of possible standby or intterrupt/dataready mode.
	Wire.endTransmission();

	Wire.beginTransmission(ADXL355_ADDRESS);
	Wire.write(ADXL355_RANGE);
	Wire.write(ADXL355_RANGE_2G); //sets the range to 2G, allowing to measure from -2G to 2G.
	Wire.endTransmission();

	Wire.beginTransmission(ADXL355_ADDRESS);
	Wire.write(ADXL355_FILTER);
	Wire.write(ADXL355_LPF); 	// Sets the ODR and LPF to a certain value
	Wire.endTransmission();
	
	delay(1000);  
}



void loop()
{	
	/////////////////////////////////////////////////// ADXL355 /////////////////////////////////////////////////////
	if (adxl_odr_timer_check > ADXL355_ODR_RATE)
	{
		Wire.beginTransmission(ADXL355_ADDRESS); // transmit to device
		Wire.write(byte(ADXL355_XDATA3));      	 // sets register pointer to echo #1 register, since xdata3 is the lowest, it will move on to xdata2,1, and then the y and z axis..
		Wire.endTransmission();      			 // stop transmitting
		Wire.requestFrom(ADXL355_ADDRESS, 9);    // request 9 bytes from slave device ADXL355

		if (Wire.available() >= 9) 
		{ // if nine bytes were received  
			adxl355_xreading3 = Wire.read();    
			adxl355_xreading2 = Wire.read();   
			adxl355_xreading1 = Wire.read();
			adxl355_yreading3 = Wire.read();    
			adxl355_yreading2 = Wire.read();   
			adxl355_yreading1 = Wire.read();
			adxl355_zreading3 = Wire.read();    
			adxl355_zreading2 = Wire.read();   
			adxl355_zreading1 = Wire.read();
		}
  
		xdata = (adxl355_xreading1 >> 4) + (adxl355_xreading2 << 4) + (adxl355_xreading3 << 12); // moves the bits from the three bytes in the right place.
		ydata = (adxl355_yreading1 >> 4) + (adxl355_yreading2 << 4) + (adxl355_yreading3 << 12);
		zdata = (adxl355_zreading1 >> 4) + (adxl355_zreading2 << 4) + (adxl355_zreading3 << 12); // here you start having actual data!

		// We are working with two complements values. Since giving a value in bytes is hard to otherwise converse to negative values, while you can have negative acceleration..
		if (xdata >= 0x80000) { xdata = -0x100000 + xdata; }
		if (ydata >= 0x80000) { ydata = -0x100000 + ydata; }
		if (zdata >= 0x80000) { zdata = -0x100000 + zdata; }

		adxl_odr_timer_check = 0;
	}

	/////////////////////////////////////////////////// ADXL1002 ////////////////////////////////////////////////////

	if (adxl1002_adc_timer > ADXL1002_SAMPLING_RATE)
	{
		//adc things
		adxl1002_value = adc->adc0->analogRead(ADXL1002_ADC_PIN);
		adxl1002_value = adxl1002_value * 3.3 / adc->adc0->getMaxValue();

		if (adc->adc0->fail_flag != ADC_ERROR::CLEAR)
		{
			Serial.print("ADXL1002 error: ");
			Serial.println(getStringADCError(adc->adc0->fail_flag));

			adc->resetError();
		}

		adxl1002_adc_timer = 0;
	}

	/////////////////////////////////////////// Print / Debugging section ///////////////////////////////////////////

	if (print_timer > 1000)
	{
		Serial.print("ADXL355 output: ");
		Serial.print("In milli g's: ");
    	Serial.print("x: "); Serial.print(xdata/256); Serial.print(" "); // Printed in a way usable by Serial plotter.
    	Serial.print("y: ");Serial.print(ydata/256); Serial.print(" "); // While a small offset is possible, taking the value and dividing it by 256 should give values in milli g's.
    	Serial.print("z: ");Serial.println(zdata/256); Serial.print(" "); 	

		Serial.print("ADXL1002 output: ");
		Serial.println(adxl1002_value);
		
		print_timer = 0;
	}

}