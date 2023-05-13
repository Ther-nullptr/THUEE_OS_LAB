#ifndef RESULT_HPP
#define RESULT_HPP

struct Result
{
    int in_time;
    int stop_time;
    int response_begin_time;
    int response_end_time;
    char event_name;
    bool is_interrupted;
};

#endif // !RESULT_HPP