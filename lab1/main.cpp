#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

#include "engine.hpp"

int main(int argc, char **argv)
{
    int n_servers = 5;
    std::string test_file_name = "test.txt";

    if (argc > 1)
    {   
        // server numbers
        n_servers = std::stoi(argv[1]);
        std::cout << "Number of servers: " << n_servers << std::endl;
    }
    else
    {
        std::cout << "use default settings" << "n_servers = " << n_servers << std::endl;
    }

    // open the file to read the data
    std::vector<int> start_time;
    std::vector<int> service_time;

    // read the data from the file
    std::ifstream infile(test_file_name);
    int a, b, c;
    while (infile >> a >> b >> c)
    {
        start_time.push_back(b);
        service_time.push_back(c);
    } 

    // construct the customers
    std::vector<Customer> customers;
    for (int i = 0; i < start_time.size(); ++i)
    {
        customers.push_back(Customer(i, start_time[i], service_time[i]));
    }

    // print the info
    for (int i = 0; i < customers.size(); ++i)
    {
        customers[i].print_info();
    }

    // construct the engine
    Engine engine(n_servers, customers);
    engine.execute();
}