#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <queue>

#include "event.hpp"
#include "result.hpp"
#include "edf.hpp"
#include "llf.hpp"
#include "rms.hpp"

enum class schedule_method
{
    RMS,
    EDF,
    LLF
};

int main(int argc, char **argv)
{
    std::string test_file_name = "test.txt";
    // if (argc > 1)
    // {
    //     int method = std::stoi(argv[1]);
    // }
    // else
    // {
    //     std::cout << "usage: ./main [method] (0: RMS, 1: EDF, 2: LLF)" << std::endl;
    //     return 0;
    // }

    // open the file to read the data
    char event_name = 'A';
    int index, is_cycle, in_time, period_or_stop_time, run_time, total_time;
    std::ifstream infile(test_file_name);
    infile >> total_time;
    std::priority_queue<Event, std::vector<Event>, std::less<Event>> event_queue;

    while(infile >> index >> is_cycle >> in_time >> period_or_stop_time >> run_time)
    {
        if (is_cycle)
        {
            // periodic task
            int i = 0;
            while(1)
            {
                // do something
                int actual_in_time = in_time + i * period_or_stop_time;
                if (actual_in_time > total_time)
                {
                    break;
                }
                Event event{actual_in_time, run_time, actual_in_time + period_or_stop_time, 0, event_name, 1000 / period_or_stop_time};
                event_queue.push(event);
                i++;
            }
        }
        else
        {
            // aperiodic task
            Event event{in_time, run_time, period_or_stop_time, 0, event_name};
            event_queue.push(event);
        }
        event_name++;
    } 

    //// print scheduled events
    // std::cout << "Total time: " << total_time << std::endl;
    // std::cout << "Scheduled events: " << std::endl;
    // std::cout << "event_name in_time total_run_time stop_time" << std::endl;
    // int size = event_queue.size();
    // for (int i = 0; i < size; ++i)
    // {
    //     Event event = event_queue.top();
    //     event_queue.pop();
    //     std::cout << event.event_name << " " << event.in_time << " " << event.total_run_time << " " << event.stop_time << std::endl;
    // }

    // print the result
    RMS rms;
    std::cout << "Result: " << std::endl;
    auto result_pair = rms.run(event_queue, total_time);
    auto result = result_pair.first;
    bool is_success = result_pair.second;
    if (!is_success)
    {
        std::cout << "Fail to schedule the events." << std::endl;
    }
    std::cout << "event_name in_time stop_time response_begin_time response_end_time" << std::endl;
    for (int i = 0; i < result.size(); ++i)
    {
        std::cout << result[i].event_name << " " << result[i].in_time << " " << result[i].stop_time << " " << result[i].response_begin_time << " " << result[i].response_end_time << std::endl;
    }
}