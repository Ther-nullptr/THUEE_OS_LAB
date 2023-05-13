#ifndef STRATEGY_HPP
#define STRATEGY_HPP

#include <queue>
#include <utility>
#include "event.hpp"
#include "result.hpp"

using event_queue_type = std::priority_queue<Event, std::vector<Event>, std::less<Event>>;
using result_pair = std::pair<std::vector<Result>, bool>;

// a base class for all strategies
class Strategy
{
public:
    Strategy() {}
    virtual ~Strategy() {}
    virtual result_pair run(event_queue_type &events, int total_time) = 0;
    virtual void preempt(int preempt_time) = 0;

protected:
    bool is_running = false;
    bool succeed = true;
    bool event_arrive = false;
    Event current_event;
    std::vector<Result> results;
};

#endif // !STRATEGY_HPP