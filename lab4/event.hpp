#ifndef EVENT_HPP
#define EVENT_HPP

struct Event
{
    // basic info
    int index;
    int in_time;
    int total_run_time;
    int stop_time;
    char event_name;

    int time_pointer; // a pointer to the current run time

    int priority; // only for RMS
    int laxity; // only for LLF

    bool operator < (const Event &b) const
    {
        return in_time > b.in_time;
    }
};

#endif // !EVENT_HPP