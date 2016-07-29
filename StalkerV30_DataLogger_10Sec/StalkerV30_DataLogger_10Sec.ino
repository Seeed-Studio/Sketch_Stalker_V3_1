//Data logger Demonstration using Seeeduino Stalker v3.0. Logs Battery Voltage every 10 seconds to DATALOG.CSV file
//Use this demo code to implement your Datalogger functionality, add additional sensors.

//1.Solder P3 and P2 PCB jumper pads
//2.Compile and upload the sketch
//3.See if everything works fine using Serial Monitor.
//4.Remove all Serial port code, recompile the sketch and upload.
// This reduces power consumption during battery mode.

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/power.h>
#include <Wire.h>
#include <DS1337.h>
#include <SPI.h>
#include <SD.h>

//The following code is taken from sleep.h as Arduino Software v22 (avrgcc) in w32 does not have the latest sleep.h file
#define sleep_bod_disable() \
{ \
  uint8_t tempreg; \
  __asm__ __volatile__("in %[tempreg], %[mcucr]" "\n\t" \
                       "ori %[tempreg], %[bods_bodse]" "\n\t" \
                       "out %[mcucr], %[tempreg]" "\n\t" \
                       "andi %[tempreg], %[not_bodse]" "\n\t" \
                       "out %[mcucr], %[tempreg]" \
                       : [tempreg] "=&d" (tempreg) \
                       : [mcucr] "I" _SFR_IO_ADDR(MCUCR), \
                         [bods_bodse] "i" (_BV(BODS) | _BV(BODSE)), \
                         [not_bodse] "i" (~_BV(BODSE))); \
}


DS1337 RTC; //Create RTC object for DS1337 RTC
static uint8_t prevSecond=0;
static DateTime interruptTime;

static uint16_t interruptInterval = 10; //Seconds. Change this to suitable value.

void setup ()
{
     /*Initialize INT0 pin for accepting interrupts */
    PORTD |= 0x04;
    DDRD &=~ 0x04;

    Wire.begin();
    Serial.begin(57600);
    RTC.begin();

    pinMode(4,OUTPUT);//SD Card power control pin. LOW = On, HIGH = Off
    digitalWrite(4,LOW); //Power On SD Card.

    Serial.print("Load SD card...");

    // Check if SD card can be intialized.
    if (!SD.begin(10))  //Chipselect is on pin 10
    {
        Serial.println("SD Card could not be intialized, or not found");
        return;
    }
    Serial.println("SD Card found and initialized.");


    attachInterrupt(0, INT0_ISR, LOW); //Only LOW level interrupt can wake up from PWR_DOWN
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    //Enable Interrupt
    //RTC.enableInterrupts(EveryMinute); //interrupt at  EverySecond, EveryMinute, EveryHour
    // or this
    DateTime  start = RTC.now();
    interruptTime = DateTime(start.get() + interruptInterval); //Add interruptInterval in seconds to start time
}

void loop ()
{
    ////////////////////// START : Application or data logging code//////////////////////////////////

    float voltage;
    int BatteryValue;

    BatteryValue = analogRead(A7);
    voltage = BatteryValue * (1.1 / 1024)* (10+2)/2;  //Voltage divider

    DateTime now = RTC.now(); //get the current date-time
    if((now.second()) !=  prevSecond )
    {
        //print only when there is a change
        Serial.print(now.year(), DEC);
        Serial.print('/');
        Serial.print(now.month(), DEC);
        Serial.print('/');
        Serial.print(now.date(), DEC);
        Serial.print(' ');
        Serial.print(now.hour(), DEC);
        Serial.print(':');
        Serial.print(now.minute(), DEC);
        Serial.print(':');
        Serial.print(now.second(), DEC);
        Serial.print("   ");
        Serial.print(voltage);
        Serial.print(" V");
        Serial.println();
    }
    prevSecond = now.second();

    //|||||||||||||||||||Write to Disk||||||||||||||||||||||||||||||||||
    File logFile = SD.open("DATALOG.CSV", FILE_WRITE);

    if(logFile) {

        logFile.print(now.year(), DEC);
        logFile.print('/');
        logFile.print(now.month(), DEC);
        logFile.print('/');
        logFile.print(now.date(), DEC);
        logFile.print(',');
        logFile.print(now.hour(), DEC);
        logFile.print(':');
        logFile.print(now.minute(), DEC);
        logFile.print(':');
        logFile.print(now.second(), DEC);
        logFile.print(',');
        logFile.println(voltage);
        logFile.close();
    }

    //|||||||||||||||||||Write to Disk||||||||||||||||||||||||||||||||||

    RTC.clearINTStatus(); //This function call is a must to bring /INT pin HIGH after an interrupt.
    RTC.enableInterrupts(interruptTime.hour(),interruptTime.minute(),interruptTime.second());    // set the interrupt at (h,m,s)
    attachInterrupt(0, INT0_ISR, LOW);  //Enable INT0 interrupt (as ISR disables interrupt). This strategy is required to handle LEVEL triggered interrupt


    ////////////////////////END : Application code ////////////////////////////////

    //\/\/\/\/\/\/\/\/\/\/\/\/Sleep Mode and Power Down routines\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\

    //Power Down routines
    cli();
    sleep_enable();      // Set sleep enable bit
    sleep_bod_disable(); // Disable brown out detection during sleep. Saves more power
    sei();

    digitalWrite(4,HIGH); //Power Off SD Card.

    Serial.println("\nSleeping");
    delay(10); //This delay is required to allow print to complete
    //Shut down all peripherals like ADC before sleep. Refer Atmega328 manual
    power_all_disable(); //This shuts down ADC, TWI, SPI, Timers and USART
    sleep_cpu();         // Sleep the CPU as per the mode set earlier(power down)
    sleep_disable();     // Wakes up sleep and clears enable bit. Before this ISR would have executed
    power_all_enable();  //This shuts enables ADC, TWI, SPI, Timers and USART
    delay(10); //This delay is required to allow CPU to stabilize
    Serial.println("Awake from sleep");
    digitalWrite(4,LOW); //Power On SD Card.

    //\/\/\/\/\/\/\/\/\/\/\/\/Sleep Mode and Power Saver routines\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\

}


//Interrupt service routine for external interrupt on INT0 pin conntected to DS1337 /INT
void INT0_ISR()
{
    //Keep this as short as possible. Possibly avoid using function calls
    detachInterrupt(0);
    interruptTime = DateTime(interruptTime.get() + interruptInterval);  //decide the time for next interrupt, configure next interrupt
}

