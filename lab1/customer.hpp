#include "semaphore.hpp"

#ifndef CUSTOMER_HPP
#define CUSTOMER_HPP

class Customer
{
public:
    Customer &operator=(const Customer &) = delete;
    ~Customer() = default;

    Customer(int index, int start_time, int service_time): index(index), start_time(start_time), service_time(service_time), sem(0, 1)
    {
        
    }

    Customer(const Customer &customer): index(customer.index), start_time(customer.start_time), service_time(customer.service_time), sem(customer.sem)
    {
        
    }

    void print_info()
    {
        std::cout << "Customer " << index << " start at " << start_time << ", service time " << service_time << std::endl;
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

#endif // CUSTOMER_HPP