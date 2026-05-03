# Plastic Scanner — nRF52 DK + Custom PCB (OsloMet)
Embedded C firmware for a plastic scanner running on Zephyr RTOS on the Nordic nRF52 DK, interfacing with a custom PCB. Developed as part of the Hardware Programming (Maskinnær Programmering) course at OsloMet, taken in spring 2025. The PCB is designed by the [Plastic Scanner Project](https://plasticscanner.com/), originally made for Arduino. This project also mimicks some of the code written for the Arduino implementation. In accordance with Nordic Semiconductors recommendation, the project was started from a template. Namely, the BMI270 sensor sample. Only the edited files have been uploaded.

* src/main.c
* app.overlay
* prj.conf

## Report
The code was written throughout the course. The reports in the reports folder (1-5) document the progression. LaTeX-files for the reports have also been included in a separate folder.
1. Plastic scanner PCB initial checks
2. Mimicking of original project code
3. Control of LED-driver
4. Control af ADC
5. System integration and testing
