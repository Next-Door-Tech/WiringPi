// WiringPi test program: Time functions
// Compile: gcc -Wall wiringpi_test10_time.c -o wiringpi_test10_time -lwiringPi

#include "wpi_test.h"
#include "../../version.h"
#include <unistd.h>

int main (void) {
  int major, minor;
  wiringPiVersion(&major, &minor);

  CheckSame("version major", major, VERSION_MAJOR);
  CheckSame("version minor", minor, VERSION_MINOR);


  CheckNotSame("wiringPiSetupGpio: ", wiringPiSetupGpio(), -1);
  __uint32_t Start_ms, duration_ms, error_ms;
  __uint32_t Start_us, duration_us, error_us;
  __uint32_t Start_ns, duration_ns, error_ns;

  for (int old=0; old<=1; old++) {
    for (int delay_time=1; delay_time<=2048; delay_time*=2) {
      //printf("sleep for %d sec\n", sleep_time); fflush(stdout);

      printf("Sleeping with %s for %d msec\n", old ? "old delay" : "new delay", delay_time);
      fflush(stdout);
    
      Start_ms = millis();
      Start_us = micros();
      Start_ns = nanos();
      //sleep(sleep_time);
      if(old) {
        delayOld(delay_time);
      } else {
        delay(delay_time);
      }
      duration_ns = nanos()-Start_ns;
      duration_us = micros()-Start_us;
      duration_ms = millis()-Start_ms;
      error_ms = duration_ms - (delay_time);
      error_us = duration_us - (delay_time*1000);
      error_ns = duration_ns - (delay_time*1000000);
      
      /*
      CheckSame("milliseconds: ", duration_ms, 1000);
      CheckSame("microseconds: ", duration_us, 1000000);
      CheckSame("nanoseconds: ", duration_ns, 1000000000);
      */

      // error 1 ms = ok
      CheckBetweenDouble("milliseconds deviation ms: ", error_ms, 0, 0);
      CheckBetweenDouble("microseconds deviation us: ", error_us, 0, 3);
      CheckBetweenDouble("nanoseconds deviation  ns: ", error_ns, 0, 3000);
    }
  }

  return UnitTestState();
}