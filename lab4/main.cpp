#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

#include "event.hpp"

enum class schedule_method
{
    RMS,
    EDF,
    LLF
};

int main(int argc, char **argv)
{
    std::string test_file_name = "test.txt";
    if (argc > 1)
    {
        int method = std::stoi(argv[1]);
    }
    else
    {
        std::cout << "usage: ./main [method] (0: RMS, 1: EDF, 2: LLF)" << std::endl;
        return 0;
    }

    // open the file to read the data
    char event_name = 'A';
    int index, is_cycle, in_time, period_or_stop_time, run_time, total_time;
    std::ifstream infile(test_file_name);
    infile >> total_time;
    std::vector<Event> event_vector;

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
                Event event{actual_in_time, run_time, actual_in_time + period_or_stop_time, 0, event_name};
                event_vector.push_back(event);
                i++;
            }
        }
        else
        {
            // aperiodic task
            Event event{in_time, run_time, period_or_stop_time, 0, event_name};
            event_vector.push_back(event);
        }
        event_name++;
    }

    // print scheduled events
    std::cout << "Total time: " << total_time << std::endl;
    std::cout << "Scheduled events: " << std::endl;
    std::cout << "event_name in_time total_run_time stop_time" << std::endl;
    for (int i = 0; i < event_vector.size(); ++i)
    {
        std::cout << event_vector[i].event_name << " " << event_vector[i].in_time << " " << event_vector[i].total_run_time << " " << event_vector[i].stop_time << std::endl;
    }
}