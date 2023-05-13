#ifndef LLF_HPP
#define LLF_HPP

#include <vector>
#include <queue>
#include "event.hpp"
#include "result.hpp"

using event_queue_type = std::priority_queue<Event, std::vector<Event>, std::less<Event>>;
using result_pair = std::pair<std::vector<Result>, bool>;

// compare function for priority queue in LLF algorithm
struct llf_cmp
{
    bool operator()(const Event &a, const Event &b)
    {
        if (a.laxity == b.laxity)
        {
            return a.in_time > b.in_time;
        }
        return a.laxity > b.laxity;
    }
};

class LLF
{
public:
    LLF() {}

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
                // event.laxity = event.stop_time - event.total_run_time;
                event_schedule_queue.push(event);
                event_arrive = true;
                if (events.empty())
                {
                    break;
                }
                event = events.top();
            }

            if (event_arrive)
            {
                // update all the laxity in event schedule queue
                std::priority_queue<Event, std::vector<Event>, llf_cmp> temp_queue;
                while (!event_schedule_queue.empty())
                {
                    Event temp_event = event_schedule_queue.top();
                    event_schedule_queue.pop();
                    temp_event.laxity = temp_event.stop_time - (i - 1) - (temp_event.total_run_time - temp_event.time_pointer);
                    temp_queue.push(temp_event);
                }
                event_schedule_queue = temp_queue;
                current_event.laxity = current_event.stop_time - (i - 1) - (current_event.total_run_time - current_event.time_pointer);
                event_arrive = false;
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

                // detect preemption
                if (!event_schedule_queue.empty())
                {
                    Event next_event = event_schedule_queue.top();
                    if (next_event.laxity < current_event.laxity && !(current_event.time_pointer == current_event.total_run_time))
                    {
                        Result result{current_event.in_time, current_event.stop_time, start_time - 1, i - 1, current_event.event_name, 1};
                        current_event.time_pointer = current_event.time_pointer - 1;
                        start_time = i; 
                        next_event.time_pointer = next_event.time_pointer + 1;
                        event_schedule_queue.pop();
                        event_schedule_queue.push(current_event);
                        results.push_back(result);
                        current_event = next_event;
                    }
                }

                if (i > current_event.stop_time) // fail to schedule
                {
                    succeed = false;
                    break;
                }

                if (current_event.time_pointer == current_event.total_run_time) // finish running
                {
                    Result result{current_event.in_time, current_event.stop_time, start_time - 1, i, current_event.event_name, 0};
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
    bool event_arrive = false;
    Event current_event;
    std::vector<Result> results;
    std::priority_queue<Event, std::vector<Event>, llf_cmp> event_schedule_queue;
};

#endif // !LLF_HPP