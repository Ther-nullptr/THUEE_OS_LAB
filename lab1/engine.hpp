#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <initializer_list>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "customer.hpp"
#include "semaphore.hpp"

#ifndef ENGINE_HPP
#define ENGINE_HPP

#define DEBUG
#define IN_BANK 0
#define BEGIN_SERVE 1
#define LEAVE_BANK 2
#define SERVE_ID 3

class Engine
{
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

public:
    Engine(int server_num, std::vector<Customer> customers): server_num(server_num), customers(customers), served_customer_num(0), begin_serve_sem(0, customers.size()), time_slice(time_slice = 100) {}

    ~Engine()
    {
        print_thread_safely({"Engine is destructing"});
    }

    void execute()
    {
        start_time = get_time_stamp_milliseconds();

        customer_served_info.reserve(customers.size());
        for (int i = 0; i < customers.size(); ++i)
        {
            std::vector<int> single_customer_served_info(4);
            customer_served_info.push_back(single_customer_served_info);
        }

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

        print_thread_safely({"All customers have been served"});
        begin_serve_sem.WakeUpAll();

        for (int i = 0; i < server_num; ++i)
        {
            if (server_threads[i].joinable())
            {
                server_threads[i].join();
            }
        }

        output_result();
    }

private:
    void run_customer(Customer& customer)
    {
        int wait_time = customer.get_start_time();
        print_thread_safely({"Customer ", std::to_string(customer.get_index()), " will come after ", std::to_string(wait_time), " time slides"});
        // wait some time, using std::chrono
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_time * time_slice));

        // get the number (which means enqueue)
        {
            std::unique_lock<std::mutex> lock(enqueue_mtx);
            customer_queue.emplace(&customer);
            begin_serve_sem.Up();
            print_thread_safely({"Customer ", std::to_string(customer.get_index()), " is entering the bank"});
            customer_served_info[customer.get_index()][IN_BANK] = get_time_slice();
        }

        // wait the service
        customer.down();

        // leave the bank
        print_thread_safely({"Customer ", std::to_string(customer.get_index()), " is leaving the bank"});
        customer_served_info[customer.get_index()][LEAVE_BANK] = get_time_slice();
    };

    void run_server(int server_id)
    {
        while (true)
        {
            // wait for the queue to be not empty
            begin_serve_sem.Down();

            if (detect_stopable())
            {
                print_thread_safely({"Server ", std::to_string(server_id), " is stopping"});
                break;
            }

            // dequeue
            Customer* customer_ptr = nullptr;
            {
                std::unique_lock<std::mutex> lock(dequeue_mtx);
                customer_ptr = customer_queue.front();
                customer_queue.pop();
                print_thread_safely({"Server ", std::to_string(server_id), " is serving customer ", std::to_string(customer_ptr->get_index())});
                customer_served_info[customer_ptr->get_index()][BEGIN_SERVE] = get_time_slice();
                customer_served_info[customer_ptr->get_index()][SERVE_ID] = server_id;
            }

            // service
            std::this_thread::sleep_for(std::chrono::milliseconds(customer_ptr->get_service_time() * time_slice));

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
        // get the time hh:ss:ms
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_c), "%T") << '.' << std::setfill('0') << std::setw(3) << ms.count();
        std::string str = "[" + ss.str() + "] " + "(time step:" + std::to_string(get_time_slice()) + ") " ;
        
        for (auto it = str_list.begin(); it != str_list.end(); ++it)
        {
            str += *it;
        }
        std::cout << str << std::endl;
    }

    int64_t get_time_stamp_milliseconds()
    {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        return ms.count();
    }

    int get_time_slice()
    {
        int64_t now_time = get_time_stamp_milliseconds();
        int64_t time_diff = now_time - start_time;
        return std::round((float)time_diff / time_slice);
    }

    void output_result()
    {
        // output the result into a file, like a table, use customer_served_info
        std::ofstream fout("output.txt");
        for (int i = 0; i < customers.size(); ++i)
        {
            fout << i << " " \
                 << customer_served_info[i][IN_BANK] << " " << customer_served_info[i][BEGIN_SERVE] << " " \
                 << customer_served_info[i][LEAVE_BANK] << " " << customer_served_info[i][SERVE_ID] << std::endl;
        }
    }

private:
    int server_num;
    int time_slice;
    int64_t start_time;
    std::atomic<int> served_customer_num;
    std::vector<Customer> customers;
    std::vector<std::thread> server_threads;
    std::vector<std::thread> customer_threads;
    std::vector<std::vector<int>> customer_served_info;
    std::queue<Customer*> customer_queue;
    mutable std::mutex enqueue_mtx;
    mutable std::mutex dequeue_mtx;
    mutable std::mutex detect_mtx;
    mutable std::mutex print_mtx;
    Semaphore begin_serve_sem;
};

#endif // ENGINE_HPP