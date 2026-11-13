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

We need a voltage divider circuit so that the ADC can tur the current that runs through the photoresistor into a measurable voltage. If the photoresistor is directly connected to the board, there would be nothing to measure. 

please see circuit diagrams in /adc. 

# DAC 
The highest DAC resolution available is **12 bits**. For \(B = 9\):

$$
V_{\text{out}} = \frac{B}{2^{12}} V_{\text{ref}}
= \frac{9}{4096} V_{\text{ref}}
$$


DAC sampling rate: A higher DAC sampling rate means more samples per cycle which means a smoother approximation of a waveform. Lower sampling rate produces larger steps and more distortion. The sampling rate must be greater than twice the highest signal frequency to avoid aliasing.

Sine waves sound smooth and only contain the fundamental frequency.
Square waves sound buzzy and contain the fundamental and odd harmonics.
Sawtooth waves sound bright and raspy and contain the fundamental, even, and odd harmonics. 

The capacitor blocks the DAC signal's DC offset when passing the audio signal. This prevents steady DC current through the speaker or earphones. It forms a high pass filter with the load resistance. 