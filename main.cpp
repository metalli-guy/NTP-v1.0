#include "includesdefinesvariables.h"
#include "mbed.h"
#include "ntp_gettime.h"
uint64_t elapsedtime;
uint32_t remsecs;
uint32_t tm_hour;
uint32_t tm_min;
uint32_t tm_sec;
time_t seconds;
int millis;
int i;
Timer t;
PwmOut pwm(p21);

int main(void) {
  NTP_gettime();
  while (true) {
    t.start();
    while (i < 10) {
      // pwm.period(1);
      // pwm = 0.5;
      i++;
      seconds = time(NULL);
      remsecs = seconds % 86400;
      tm_hour = remsecs / 3600;
      tm_min = remsecs / 60 % 60;
      tm_sec = remsecs % 60;
      char buffer2[32];
      millis =
          duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count() %
          1000;
      // printf("delaytime %u\n", millis);
      strftime(buffer2, 32, "%Y-%m-%d %H:%M:%S:", localtime(&seconds));
      //printf("i %u\n",i);
      printf("%u:%u:%u:%u\n", tm_hour, tm_min, tm_sec, millis);
      // printf("RTC = %s\n", buffer2);
      ThisThread::sleep_for(1000ms);
      t.reset();
    }
    i = 0;
    //main();
  }
}