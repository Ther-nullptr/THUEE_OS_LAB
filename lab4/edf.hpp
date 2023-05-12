#ifndef EDF_HPP
#define EDF_HPP

#include <vector>
#include <queue>
#include "event.hpp"
#include "result.hpp"

using event_queue_type = std::priority_queue<Event, std::vector<Event>, std::less<Event>>;

// compare function for priority queue in EDF algorithm
struct cmp
{
    bool operator()(const Event &a, const Event &b)
    {
        if (a.stop_time == b.stop_time)
        {
            return a.in_time > b.in_time;
        }
        return a.stop_time > b.stop_time;
    }
};

class EDF
{
public:
    EDF(event_queue_type &events) {}

    void run(event_queue_type &events, int total_time)
    {
        for (int i = 0; i <= total_time; i++)
        {
            if (events.empty())
            {
                break;
            }

            Event event = events.top();
            while (event.in_time == i)
            {
                events.pop();
                event_schedule_queue.push(event);
                if (events.empty())
                {
                    break;
                }
                event = events.top();
            }
        }
    }

private:
    std::vector<Result> results;
    std::priority_queue<Event, std::vector<Event>, cmp> event_schedule_queue;
};


#endif // !EDF_HPP