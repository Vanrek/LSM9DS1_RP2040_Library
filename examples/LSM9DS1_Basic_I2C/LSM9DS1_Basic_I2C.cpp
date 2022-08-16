/*****************************************************************
  LSM9DS1_Basic_I2C.ino
  SFE_LSM9DS1 Library Simple Example Code - I2C Interface
  Jim Lindblom @ SparkFun Electronics
  Original Creation Date: April 30, 2015
  https://github.com/sparkfun/LSM9DS1_Breakout

  The LSM9DS1 is a versatile 9DOF sensor. It has a built-in
  accelerometer, gyroscope, and magnetometer. Very cool! Plus it
  functions over either SPI or I2C.

  This Arduino sketch is a demo of the simple side of the
  SFE_LSM9DS1 library. It'll demo the following:
  How to create a LSM9DS1 object, using a constructor (global
  variables section).
  How to use the begin() function of the LSM9DS1 class.
  How to read the gyroscope, accelerometer, and magnetometer
  using the readGryo(), readAccel(), readMag() functions and
  the gx, gy, gz, ax, ay, az, mx, my, and mz variables.
  How to calculate actual acceleration, rotation speed,
  magnetic field strength using the calcAccel(), calcGyro()
  and calcMag() functions.
  How to use the data from the LSM9DS1 to calculate
  orientation and heading.

  Hardware setup: This library supports communicating with the
  LSM9DS1 over either I2C or SPI. This example demonstrates how
  to use I2C. The pin-out is as follows:
	LSM9DS1 --------- Arduino
	 SCL ---------- SCL (A5 on older 'Duinos')
	 SDA ---------- SDA (A4 on older 'Duinos')
	 VDD ------------- 3.3V
	 GND ------------- GND
  (CSG, CSXM, SDOG, and SDOXM should all be pulled high.
  Jumpers on the breakout board will do this for you.)

  The LSM9DS1 has a maximum voltage of 3.6V. Make sure you power it
  off the 3.3V rail! I2C pins are open-drain, so you'll be
  (mostly) safe connecting the LSM9DS1's SCL and SDA pins
  directly to the Arduino.

  Development environment specifics:
	IDE: Arduino 1.6.3
	Hardware Platform: SparkFun Redboard
	LSM9DS1 Breakout Version: 1.0

  This code is beerware. If you see me (or any other SparkFun
  employee) at the local, and you've found our code helpful,
  please buy us a round!

  Distributed as-is; no warranty is given.
*****************************************************************/
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/float.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

#include "LSM9DS1_RP2040_Library/src/LSM9DS1.h"

#define PI 3.14159265358979323846264338327950288419716939937510582f

//////////////////////////
// LSM9DS1 Library Init //
//////////////////////////
// Use the LSM9DS1 class to create an object. [imu] can be
// named anything, we'll refer to that throught the sketch.
LSM9DS1 imu;

////////////////////////////
// Sketch Output Settings //
////////////////////////////
#define PRINT_CALCULATED
//#define PRINT_RAW
#define PRINT_SPEED 250             // 250 ms between prints
static unsigned long lastPrint = 0; // Keep track of print time

// Earth's magnetic field varies by location. Add or subtract
// a declination to get a more accurate heading. Calculate
// your's here:
// http://www.ngdc.noaa.gov/geomag-web/#declination
#define DECLINATION -8.58 // Declination (degrees) in Boulder, CO.

// Function definitions
void printGyro();
void printAccel();
void printMag();
void printAttitude(float ax, float ay, float az, float mx, float my, float mz);

int main(){
    stdio_init_all();

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(i2c_default, 400 * 1000);

    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);

    if (imu.begin() == false) // with no arguments, this uses default addresses (AG:0x6B, M:0x1E) and i2c port (Wire).
    {
        puts("Failed to communicate with LSM9DS1.");
        puts("Double-check wiring.");
        puts("Default settings in this sketch will "
             "work for an out of the box LSM9DS1 "
             "Breakout, but may need to be modified "
             "if the board jumpers are.");
        while (1)
            ;
    }

    while (1)
    {

        // Update the sensor values whenever new data is available
        if (imu.gyroAvailable())
        {
            // To read from the gyroscope,  first call the
            // readGyro() function. When it exits, it'll update the
            // gx, gy, and gz variables with the most current data.
            imu.readGyro();
        }
        if (imu.accelAvailable())
        {
            // To read from the accelerometer, first call the
            // readAccel() function. When it exits, it'll update the
            // ax, ay, and az variables with the most current data.
            imu.readAccel();
        }
        if (imu.magAvailable())
        {
            // To read from the magnetometer, first call the
            // readMag() function. When it exits, it'll update the
            // mx, my, and mz variables with the most current data.
            imu.readMag();
        }

        if ((lastPrint + PRINT_SPEED) < to_ms_since_boot(get_absolute_time()))
        {
            printGyro();  // Print "G: gx, gy, gz"
            printAccel(); // Print "A: ax, ay, az"
            printMag();   // Print "M: mx, my, mz"
            // Print the heading and orientation for fun!
            // Call print attitude. The LSM9DS1's mag x and y
            // axes are opposite to the accelerometer, so my, mx are
            // substituted for each other.
            printAttitude(imu.ax, imu.ay, imu.az,
                          -imu.my, -imu.mx, imu.mz);
            puts("");

            lastPrint = to_ms_since_boot(get_absolute_time()); // Update lastPrint time
        }
    }

    return 0;
}

void printGyro()
{
    // Now we can use the gx, gy, and gz variables as we please.
    // Either print them as raw ADC values, or calculated in DPS.
    puts("G: ");
#ifdef PRINT_CALCULATED
    // If you want to print calculated values, you can use the
    // calcGyro helper function to convert a raw ADC value to
    // DPS. Give the function the value that you want to convert.
    printf("%.2f, ", imu.calcGyro(imu.gx));
    printf("%.2f, ", imu.calcGyro(imu.gy));
    printf("%.2f  deg/s\n", imu.calcGyro(imu.gz));
#elif defined PRINT_RAW
    printf("%d, ", imu.gx);
    printf("%d, ", imu.gy);
    printf("%d\n", imu.gz);
#endif
}

void printAccel()
{
    // Now we can use the ax, ay, and az variables as we please.
    // Either print them as raw ADC values, or calculated in g's.
    puts("A: ");
#ifdef PRINT_CALCULATED
    // If you want to print calculated values, you can use the
    // calcAccel helper function to convert a raw ADC value to
    // g's. Give the function the value that you want to convert.
    printf("%.2f, ", imu.calcAccel(imu.ax));
    printf("%.2f, ", imu.calcAccel(imu.ay));
    printf("%.2f  g\n", imu.calcAccel(imu.az));
#elif defined PRINT_RAW
    printf("%d, ", imu.ax);
    printf("%d, ", imu.ay);
    printf("%d\n", imu.az);
#endif
}

void printMag()
{
    // Now we can use the mx, my, and mz variables as we please.
    // Either print them as raw ADC values, or calculated in Gauss.
    puts("M: ");
#ifdef PRINT_CALCULATED
    // If you want to print calculated values, you can use the
    // calcMag helper function to convert a raw ADC value to
    // Gauss. Give the function the value that you want to convert.
    printf("%.2f, ", imu.calcMag(imu.mx));
    printf("%.2f, ", imu.calcMag(imu.my));
    printf("%.2f  gauss\n", imu.calcMag(imu.mz));
#elif defined PRINT_RAW
    printf("%d, ", imu.mx);
    printf("%d, ", imu.my);
    printf("%d\n",imu.mz);
#endif
}

// Calculate pitch, roll, and heading.
// Pitch/roll calculations take from this app note:
// https://web.archive.org/web/20190824101042/http://cache.freescale.com/files/sensors/doc/app_note/AN3461.pdf
// Heading calculations taken from this app note:
// https://web.archive.org/web/20150513214706/http://www51.honeywell.com/aero/common/documents/myaerospacecatalog-documents/Defense_Brochures-documents/Magnetic__Literature_Application_notes-documents/AN203_Compass_Heading_Using_Magnetometers.pdf
void printAttitude(float ax, float ay, float az, float mx, float my, float mz)
{
    float roll = atan2(ay, az);
    float pitch = atan2(-ax, sqrt(ay * ay + az * az));

    float heading;
    if (my == 0)
        heading = (mx < 0) ? PI : 0;
    else
        heading = atan2(mx, my);

    heading -= DECLINATION * PI / 180;

    if (heading > PI)
        heading -= (2 * PI);
    else if (heading < -PI)
        heading += (2 * PI);

    // Convert everything from radians to degrees:
    heading *= 180.0 / PI;
    pitch *= 180.0 / PI;
    roll *= 180.0 / PI;

    puts("Pitch, Roll: ");
    printf("%.2f, ", pitch);
    printf("%.2f, ", roll);
    printf("%.2f\n", heading);
}
