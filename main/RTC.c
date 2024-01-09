#include "RTC.h"
#include "main.h"

void set_clock(struct tm *timeinfo)
{
  struct tm time2set = {
      .tm_year = timeinfo->tm_year,
      .tm_mon = timeinfo->tm_mon,
      .tm_mday = timeinfo->tm_mday,
      .tm_hour = timeinfo->tm_hour,
      .tm_min = timeinfo->tm_min,
      .tm_sec = timeinfo->tm_sec};
  if (ds3231_set_time(&rtc_i2c, &time2set) != ESP_OK)
  {
    ESP_LOGE(pcTaskGetTaskName(0), "Could not set time.");
    while (1)
    {
      vTaskDelay(1);
    }
  }
  ESP_LOGI(pcTaskGetTaskName(0), "Set date time done");

  vTaskDelete(NULL);
}

int calculate_diff_time(struct tm start_time, struct tm end_time)
{
  int diff_time;
  int diff_hour;
  int diff_min;
  int diff_sec;
  int e_sec = 0;
  int e_min = 0;

  if (end_time.tm_sec < start_time.tm_sec)
  {
    diff_sec = end_time.tm_sec + 60 - start_time.tm_sec;
    e_sec = 1;
  }
  else
  {
    diff_sec = end_time.tm_sec - start_time.tm_sec;
  }

  if (end_time.tm_min < start_time.tm_min)
  {
    diff_min = end_time.tm_min - e_sec + 60 - start_time.tm_min;
    e_min = 1;
  }
  else
  {
    diff_min = end_time.tm_min - e_sec - start_time.tm_min;
  }

  if (end_time.tm_hour < start_time.tm_hour)
  {
    diff_hour = end_time.tm_hour - e_min + 24 - start_time.tm_hour;
  }
  else
  {
    diff_hour = end_time.tm_hour - e_min - start_time.tm_hour;
  }

  diff_time = (diff_hour * 3600 + diff_min * 60 + diff_sec) * 1000;

  return diff_time;
}

void get_tomorrow(struct tm *tomorrow, struct tm *today)
{
  int year = today->tm_year;
  int mon = today->tm_mon;
  int mday = today->tm_mday;
  int leap_year = year % 4;

  tomorrow->tm_year = today->tm_year;
  tomorrow->tm_mon = today->tm_mon;
  tomorrow->tm_mday = today->tm_mday + 1;

  if ((mday == 31) && (mon == 1))
  {
    tomorrow->tm_mon = 2;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 28) && (mon == 2))
  {
    if (leap_year != 0)
    {
      tomorrow->tm_mon = 3;
      tomorrow->tm_mday = 1;
    }
    else
    {
      tomorrow->tm_mon = 2;
      tomorrow->tm_mday = 29;
    }
    return;
  }
  if ((mday == 29) && (mon == 2))
  {
    tomorrow->tm_mon = 3;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 31) && (mon == 3))
  {
    tomorrow->tm_mon = 4;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 30) && (mon == 4))
  {
    tomorrow->tm_mon = 5;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 31) && (mon == 5))
  {
    tomorrow->tm_mon = 6;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 30) && (mon == 6))
  {
    tomorrow->tm_mon = 7;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 31) && (mon == 7))
  {
    tomorrow->tm_mon = 8;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 31) && (mon == 8))
  {
    tomorrow->tm_mon = 9;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 30) && (mon == 9))
  {
    tomorrow->tm_mon = 10;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 31) && (mon == 10))
  {
    tomorrow->tm_mon = 11;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 30) && (mon == 11))
  {
    tomorrow->tm_mon = 12;
    tomorrow->tm_mday = 1;
    return;
  }
  if ((mday == 31) && (mon == 12))
  {
    tomorrow->tm_mon = 1;
    tomorrow->tm_mday = 1;
    tomorrow->tm_year = tomorrow->tm_year + 1;
    return;
  }
  return;
}

void get_previous_day(struct tm *previous, struct tm *today)
{
  int year = today->tm_year;
  int mon = today->tm_mon;
  int mday = today->tm_mday;
  int leap_year = year % 4;

  previous->tm_year = today->tm_year;
  previous->tm_mon = today->tm_mon;
  previous->tm_mday = today->tm_mday - 1;

  if ((mday == 1) && (mon == 1))
  {
    previous->tm_mon = 12;
    previous->tm_mday = 31;
    previous->tm_year = previous->tm_year - 1;
    return;
  }
  if ((mday == 1) && (mon == 2))
  {
    previous->tm_mon = 1;
    previous->tm_mday = 31;
    return;
  }
  if ((mday == 1) && (mon == 3))
  {
    if (leap_year != 0)
    {
      previous->tm_mon = 2;
      previous->tm_mday = 28;
    }
    else
    {
      previous->tm_mon = 2;
      previous->tm_mday = 29;
    }
    return;
  }
  if ((mday == 1) && (mon == 4))
  {
    previous->tm_mon = 3;
    previous->tm_mday = 31;
    return;
  }
  if ((mday == 1) && (mon == 5))
  {
    previous->tm_mon = 4;
    previous->tm_mday = 30;
    return;
  }
  if ((mday == 1) && (mon == 6))
  {
    previous->tm_mon = 5;
    previous->tm_mday = 31;
    return;
  }
  if ((mday == 1) && (mon == 7))
  {
    previous->tm_mon = 6;
    previous->tm_mday = 30;
    return;
  }
  if ((mday == 1) && (mon == 8))
  {
    previous->tm_mon = 7;
    previous->tm_mday = 31;
    return;
  }
  if ((mday == 1) && (mon == 9))
  {
    previous->tm_mon = 8;
    previous->tm_mday = 31;
    return;
  }
  if ((mday == 1) && (mon == 10))
  {
    previous->tm_mon = 9;
    previous->tm_mday = 30;
    return;
  }
  if ((mday == 1) && (mon == 11))
  {
    previous->tm_mon = 31;
    previous->tm_mday = 10;
    return;
  }
  if ((mday == 1) && (mon == 12))
  {
    previous->tm_mon = 30;
    previous->tm_mday = 11;
    return;
  }

  return;
}