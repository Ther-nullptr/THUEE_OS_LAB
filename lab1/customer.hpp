#include "semaphore.hpp"

class Customer
{
public:
    Customer(const Customer &) = delete;
    Customer &operator=(const Customer &) = delete;
    ~Customer() = default;

    Customer(int index, int start_time, int service_time): index(index), start_time(start_time), service_time(service_time), sem(0, 0)
    {
        
    }

    void print_info()
    {
        std::cout << "Customer " << index << " start at " << start_time << " and service at " << service_time << std::endl;
    }

    void up()
    {
        sem.Up();  
    };

    void down()
    {
        sem.Down();
    };

    // get functions, not changeable
    const int get_index() const noexcept
    {
        return index;
    }

    const int get_start_time() const noexcept
    {
        return start_time;
    }

    const int get_service_time() const noexcept
    {
        return service_time;
    }

private:
    int index;
    int start_time;
    int service_time;
    Semaphore sem;
};