#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include "customer.hpp"
#include "server.hpp"
#include "semaphore.hpp"

#define DEBUG

class Engine
{
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

public:
    Engine(int server_num, std::vector<Customer> customers): server_num(server_num), customers(customers), served_customer_num(0)
    {
        for (int i = 0; i < server_num; ++i)
        {
            servers.push_back(Server(i));
        }

        // begin all the server threads
        for (int i = 0; i < server_num; ++i)
        {
            std::thread t(&Engine::run_server, this, &servers[i]);
            server_threads.push_back(std::move(t));
            server_threads[i].detach();
        }

        // begin all the customer threads
        for (int i = 0; i < customers.size(); ++i)
        {
            std::thread t(&Engine::run_customer, this, &customers[i]);
            customer_threads.push_back(std::move(t));
            customer_threads[i].detach();
        }
    }

    void run_customer(Customer& customer)
    {
        int wait_time = customer.get_start_time();
        // wait some time, using std::chrono
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time * 100));

        // get the number (which means enqueue)
        {
            std::unique_lock<std::mutex> lock(enqueue_mtx);
            customer_queue.push(customer);
        }

        // wait the service
        customer.down();
    };

    void run_server(Server& server)
    {
        while (true)
        {
            // dequeue
            std::unique_ptr<Customer> customer_ptr;
            {
                std::unique_lock<std::mutex> lock(dequeue_mtx);
                customer_ptr = std::make_unique<Customer>(customer_queue.front());
                customer_queue.pop();
                server.set_served_customer_index(customer_ptr->get_index());
#ifdef DEBUG
                std::cout << "Server " << server.get_index() << " is serving customer " << customer_ptr->get_index() << std::endl;
#endif
            }

            // service
            std::this_thread::sleep_for(std::chrono::milliseconds(customer_ptr->get_service_time() * 100));

            // finish customer thread
            customer_ptr->up();
            served_customer_num++;

            if(detect_stopable())
            {
                break;
            }
        }
    };

    bool detect_stopable()
    {
        std::unique_lock<std::mutex> lock(detect_mtx);
        return (served_customer_num == customers.size());
    }

private:
    int server_num;
    std::atomic<int> served_customer_num;
    std::vector<Customer> customers;
    std::vector<Server> servers;
    std::vector<std::thread> server_threads;
    std::vector<std::thread> customer_threads;
    std::queue<Customer> customer_queue;
    mutable std::mutex enqueue_mtx;
    mutable std::mutex dequeue_mtx;
    mutable std::mutex detect_mtx;
};