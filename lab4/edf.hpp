#ifndef EDF_HPP
#define EDF_HPP

#include <vector>
#include <queue>
#include "event.hpp"
#include "result.hpp"

using event_queue_type = std::priority_queue<Event, std::vector<Event>, std::less<Event>>;
using result_pair = std::pair<std::vector<Result>, bool>;

// compare function for priority queue in EDF algorithm
struct edf_cmp
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
    EDF() {}

    result_pair run(event_queue_type &events, int total_time)
    {
        int i = 0;
        while(1)
        {
            if (events.empty() && event_schedule_queue.empty() && !is_running)
            {
                break;
            }

            // prepare the event schedule queue
            Event event = events.top();
            while (event.in_time == i - 1)
            {
                events.pop();
                event_schedule_queue.push(event);
                if (events.empty())
                {
                    break;
                }
                event = events.top();
            }

            // execute the event
            int start_time;
            if (!is_running)
            {
                if (!event_schedule_queue.empty())
                {
                    current_event = event_schedule_queue.top();
                    event_schedule_queue.pop();
                    start_time = i; // begin to run
                    is_running = true;
                }
            }

            if (is_running)
            {
                current_event.time_pointer = current_event.time_pointer + 1;

                if (i > current_event.stop_time) // fail to schedule
                {
                    succeed = false;
                    break;
                }

                if (current_event.time_pointer == current_event.total_run_time) // finish running
                {
                    Result result{index : current_event.index, in_time : current_event.in_time, stop_time : current_event.stop_time, response_begin_time : start_time - 1, response_end_time : i, event_name : current_event.event_name, is_interrupted : 0};
                    results.push_back(result);
                    is_running = false;
                }
            }

            i++;
        }
        return std::make_pair(results, succeed);
    }

private:
    bool is_running = false;
    bool succeed = true;
    Event current_event;
    std::vector<Result> results;
    std::priority_queue<Event, std::vector<Event>, edf_cmp> event_schedule_queue;
};

#endif // !EDF_HPP