# EE186 Lab 4


# ADC 
The ADC can suppport 12 bit, 10 bit, 8 bit, and 6 bit resolutions. This comes from Table 81 on pg. 214 of the STM32 datasheet. 
The max sampling rates (for fast channels):
- 12 bit: 5.33 Msps
- 10 bit: 6.15 Msps
- 8 bit: 7.27 Msps
- 6 bit: 8.88 Msps
The max sampling rates (for slow channels):
- 12 bit: 4.21 Msps
- 10 bit: 4.71 Msps
- 8 bit: 5.33 Msps
- 6 bit: 6.15 Msps

We need a voltage divider circuit so that the ADC can tur the current that runs through the photodiode into a measurable voltage. If the photodiode is directly connected to the board, there would be nothing to measure. 

please see circuit diagrams in /adc. 

# DAC 

