#ifndef EVENT_HPP
#define EVENT_HPP

struct Event
{
    int in_time;
    int total_run_time;
    int stop_time;
    int time_pointer; // a pointer to the current run time
    char event_name;
};

#endif // !EVENT_HPP