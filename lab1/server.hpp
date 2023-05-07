#include <vector>
#include "semaphore.hpp"

class Server
{
    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;
public:
    Server(int index): index(index)
    {

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