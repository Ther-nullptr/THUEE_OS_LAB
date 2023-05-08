#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <initializer_list>
#include "customer.hpp"
#include "semaphore.hpp"

#define DEBUG

#ifndef ENGINE_HPP
#define ENGINE_HPP

class Engine
{
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

public:
    Engine(int server_num, std::vector<Customer> customers): server_num(server_num), customers(customers), served_customer_num(0), begin_serve_sem(0, customers.size())
    {
        // initialize the servers
        for (int i = 0; i < server_num; ++i)
        {
            served_customer_index.push_back(std::vector<int>());
        }
    }

    ~Engine()
    {
        print_thread_safely({"Engine is destructing"});
    }

    void execute()
    {
        // begin all the server threads
        server_threads.reserve(server_num);
        for (int i = 0; i < server_num; ++i)
        {
            server_threads.emplace_back(&Engine::run_server, this, i);
            print_thread_safely({"Server ", std::to_string(i), " is ready"}); 
        }

        // begin all the customer threads
        customer_threads.reserve(customers.size());
        for (int i = 0; i < customers.size(); ++i)
        {
            customer_threads.emplace_back(&Engine::run_customer, this, std::ref(customers[i]));
            print_thread_safely({"Customer ", std::to_string(i), " is ready"});
        }
        for (int i = 0; i < customers.size(); ++i)
        {
            customer_threads[i].join();
        }

        std::cout << "All customers have been served" << std::endl;
        begin_serve_sem.WakeUpAll();

        for (int i = 0; i < server_num; ++i)
        {
            if (server_threads[i].joinable())
            {
                server_threads[i].join();
            }
        }
    }

private:
    void run_customer(Customer& customer)
    {
        int wait_time = customer.get_start_time();
#ifdef DEBUG
        // std::cout << "Customer " << customer.get_index() << " is waiting for " << wait_time * 0.1 << " seconds" << std::endl;
        print_thread_safely({"Customer ", std::to_string(customer.get_index()), " is waiting for ", std::to_string(wait_time * 0.1), " seconds"});
#endif
        // wait some time, using std::chrono
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time * 100));

        // get the number (which means enqueue)
        {
            std::unique_lock<std::mutex> lock(enqueue_mtx);
            customer_queue.emplace(&customer);
            begin_serve_sem.Up();
#ifdef DEBUG
            // std::cout << "Customer " << customer.get_index() << " is entering the bank" << std::endl;
            print_thread_safely({"Customer ", std::to_string(customer.get_index()), " is entering the bank"});
#endif
        }

        // wait the service
        customer.down();

        // leave the bank
#ifdef DEBUG
        // std::cout << "Customer " << customer.get_index() << " is leaving the bank" << std::endl;
        print_thread_safely({"Customer ", std::to_string(customer.get_index()), " is leaving the bank"});
#endif
    };

    void run_server(int server_id)
    {
        while (true)
        {
            // wait for the queue to be not empty
            begin_serve_sem.Down();

            if (detect_stopable())
            {
                break;
            }

            // dequeue
            Customer* customer_ptr = nullptr;
            {
                std::unique_lock<std::mutex> lock(dequeue_mtx);
                customer_ptr = customer_queue.front();
                customer_queue.pop();
                // served_customer_index[server_id].push_back(customer_ptr->get_index());
#ifdef DEBUG
                // std::cout << "Server " << server.get_index() << " is serving customer " << customer_ptr->get_index() << std::endl;
                print_thread_safely({"Server ", std::to_string(server_id), " is serving customer ", std::to_string(customer_ptr->get_index())});
#endif
            }

            // service
            std::this_thread::sleep_for(std::chrono::milliseconds(customer_ptr->get_service_time() * 100));

            // finish customer thread
            customer_ptr->up();
            served_customer_num++;
        }
    };

    bool detect_stopable()
    {
        std::unique_lock<std::mutex> lock(detect_mtx);
        return (served_customer_num == customers.size());
    }

    void print_thread_safely(std::initializer_list<std::string> str_list)
    {
        std::unique_lock<std::mutex> lock(print_mtx);
        std::string str;
        for (auto it = str_list.begin(); it != str_list.end(); ++it)
        {
            str += *it;
        }
        std::cout << str << std::endl;
    }

private:
    int server_num;
    std::atomic<int> served_customer_num;
    std::vector<Customer> customers;
    std::vector<std::thread> server_threads;
    std::vector<std::thread> customer_threads;
    std::vector<std::vector<int>> served_customer_index; 
    std::queue<Customer*> customer_queue;
    mutable std::mutex enqueue_mtx;
    mutable std::mutex dequeue_mtx;
    mutable std::mutex detect_mtx;
    mutable std::mutex print_mtx;
    Semaphore begin_serve_sem;
};

#endif // ENGINE_HPP