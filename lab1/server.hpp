#include <vector>
#include "semaphore.hpp"

#ifndef SERVER_HPP
#define SERVER_HPP

class Server
{
public:
    Server(int index): index(index)
    {

    }

    Server(const Server &): index(index)
    {

    }

    ~Server()
    {
        std::cout << "Server " << index << " destructed" << std::endl;
    }

    void set_served_customer_index(int served_customer_index)
    {
        this->served_customer_index.push_back(served_customer_index);
    }

    const int get_index() const noexcept
    {
        return index;
    }

    const std::vector<int> get_served_customers() const noexcept
    {
        return served_customer_index;
    }

private:
    int index;
    std::vector<int> served_customer_index;
};

#endif // SERVER_HPP