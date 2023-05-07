#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

int main(int argc, char **argv)
{
    int n_tellers = 3;
    std::string test_file_name = "test.txt";

    if (argc > 2)
    {   
        // teller numbers
        n_tellers = std::stoi(argv[1]);
        std::cout << "Number of tellers: " << n_tellers << std::endl;

        // test file names
        std::string test_file_name = argv[2];
        std::cout << "Test file name: " << test_file_name << std::endl;
    }
    else
    {
        std::cout << "use default settings" << "n_tellers = 3, test_file_name = test.txt" << std::endl;
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

    // check the data 
    bool sorted = std::is_sorted(start_time.begin(), start_time.end());
    if (!sorted)
    {
        std::cout << "start_time is not sorted!" << std::endl;
        return 0;
    }
}