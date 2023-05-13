#ifndef RMS_HPP
#define RMS_HPP

#include <vector>
#include <queue>
#include <utility>
#include "event.hpp"
#include "result.hpp"
#include "strategy.hpp"

using event_queue_type = std::priority_queue<Event, std::vector<Event>, std::less<Event>>;
using result_pair = std::pair<std::vector<Result>, bool>;

// compare function for priority queue in RMS algorithm
struct rms_cmp
{
    bool operator()(const Event &a, const Event &b)
    {
        if (a.priority == b.priority)
        {
            return a.in_time > b.in_time;
        }
        return a.priority < b.priority;
    }
};

class RMS: public Strategy
{
public:
    RMS() {}

    result_pair run(event_queue_type &events, int total_time) override
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

                // detect preempt
                if (event_arrive)
                {
                    preempt(i);
                    event_arrive = false;
                }

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
    void preempt(int preempt_time) override
    {
        Event next_event = event_schedule_queue.top();
        if (next_event.priority > current_event.priority && !(current_event.time_pointer == current_event.total_run_time))
        {
            Result result{index : current_event.index, in_time : current_event.in_time, stop_time : current_event.stop_time, response_begin_time : start_time - 1, response_end_time : preempt_time - 1, event_name : current_event.event_name, is_interrupted : 1};
            current_event.time_pointer = current_event.time_pointer - 1;
            start_time = preempt_time; 
            next_event.time_pointer = next_event.time_pointer + 1;
            event_schedule_queue.pop();
            event_schedule_queue.push(current_event);
            results.push_back(result);
            current_event = next_event;
        }
    }

    int start_time = 0;
    std::priority_queue<Event, std::vector<Event>, rms_cmp> event_schedule_queue;
};

#endif // !RMS