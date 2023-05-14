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
    schedule_method method = schedule_method::RMS;
    std::string test_file_name = "test.txt";

    if (argc > 1)
    {
        method = std::atoi(argv[1]) == 1 ? schedule_method::RMS : std::atoi(argv[1]) == 2 ? schedule_method::EDF : schedule_method::LLF;
        std::cout << "method: " << (method == schedule_method::RMS ? "RMS" : method == schedule_method::EDF ? "EDF" : "LLF") << std::endl;
    }
    else
    {
        std::cout << "help: ./main [ RMS(1) | EDF(2) | LLF(3) ] " << std::endl;
        return 0;
    }

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
                Event event{index : i, in_time : actual_in_time, total_run_time : run_time, stop_time : actual_in_time + period_or_stop_time, event_name : event_name, time_pointer : 0, priority : 1000 / period_or_stop_time};
                event_queue.push(event);
                i++;
            }
        }
        else
        {
            // aperiodic task
            Event event{index : 0, in_time : in_time, total_run_time : run_time, stop_time : period_or_stop_time, event_name : event_name, time_pointer : 0};
            event_queue.push(event);
        }
        event_name++;
    }

    // print the result
    Strategy *strategy;
    switch (method)
    {
        case schedule_method::RMS:
            strategy = new RMS();
            break;
        case schedule_method::EDF:
            strategy = new EDF();
            break;
        case schedule_method::LLF:
            strategy = new LLF();
            break;
        default:
            std::cout << "Invalid method." << std::endl;
            return 0;
    }

    result_pair result_pair = strategy->run(event_queue, total_time);
    std::vector<Result> result = result_pair.first;
    bool is_success = result_pair.second;
    if (!is_success)
    {
        // print with red color
        std::cout << "\033[31mFail to schedule the events.\033[0m" << std::endl;
    }
    else
    {
        std::cout << "\033[32mSuccess to schedule the events.\033[0m" << std::endl;
    }

    // file to write the result
    std::ofstream outfile("result.txt");
    std::cout << "Result: " << std::endl;
    std::cout << "event_name in_time stop_time response_begin_time response_end_time" << std::endl;
    for (int i = 0; i < result.size(); ++i)
    {
        // write to file and print to console at the same time
        outfile << result[i].event_name << result[i].index << " " << result[i].in_time << " " << result[i].stop_time << " " << result[i].response_begin_time << " " << result[i].response_end_time << std::endl;
        std::cout << result[i].event_name << result[i].index << " " << result[i].in_time << " " << result[i].stop_time << " " << result[i].response_begin_time << " " << result[i].response_end_time << std::endl;
    }

    delete strategy;
}