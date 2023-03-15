# Teensy_Sensor_System

Sensor readings using Teensy 4.1

## ADXL355

- I2C
- ODR (output data rate) --> 4 kHz (with low pass filter of 1 kHz cutoff frequency)
- Analog Devices driver too complex, could not get it to work
- "Custom" driver ([random website](https://www.notion.so/Accelerometers-80cfb311752c4881bb23ff1b13aa603a?pvs=4#beaaff6d3e854049ad9442ea09ecbcb5))

## ADXL1002

- Analog
- ADC --> 1 Msps, 12 bit
