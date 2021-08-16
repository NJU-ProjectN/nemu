#ifndef __DEVICE_ALARM_H__
#define __DEVICE_ALARM_H__

#define TIMER_HZ 60

typedef void (*alarm_handler_t) ();
void add_alarm_handle(alarm_handler_t h);

#endif
