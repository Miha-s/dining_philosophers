#include <mutex>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>
#include "Timer.hpp"

using StopSignalType = std::atomic_bool;
using ForkMutexPtr = std::shared_ptr<std::mutex>;

class Philosofer
{
    int m_name;
    ForkMutexPtr m_left_fork;
    ForkMutexPtr m_right_fork;
    int m_thinking_ms;
    int m_eating_ms;
    double m_waiting_ms = 0;
public:
    Philosofer(int name, ForkMutexPtr left_fork, ForkMutexPtr right_fork, int thinking_ms = 200, int eating_ms = 200)
    : m_name(name), m_left_fork(left_fork), m_right_fork(right_fork), 
    m_thinking_ms(thinking_ms), m_eating_ms(eating_ms) {}

    void live(const StopSignalType& stop_signal)
    {
        while (!stop_signal.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_thinking_ms));

            {
                Timer timer;
                std::lock_guard<std::mutex> right_g{*m_right_fork};
                std::lock_guard<std::mutex> left_g{*m_left_fork};
                m_waiting_ms += timer.elapsedMilliseconds();
                
                std::this_thread::sleep_for(std::chrono::milliseconds(m_eating_ms));
            }
        }

        std::printf("Philosofer %d waiting seconds %f\n", m_name, m_waiting_ms/1000);
    }
};


void let_philosofers_live(const std::size_t num_philosofers, int total_running_time_s, int thinking_ms = 200, int eating_ms = 200)
{
    std::vector<ForkMutexPtr> m_forks_mutexes;
    m_forks_mutexes.resize(num_philosofers + 1);

    for(int i = 0; i < num_philosofers + 1; i++)
    {
        m_forks_mutexes[i].reset(new std::mutex);
    }

    std::vector<std::thread> m_philosofers_threads;

    StopSignalType stop_signal;

    for(int i = 0; i < num_philosofers; i++)
    {
        auto lauch_philosofer = [i, &m_forks_mutexes, &stop_signal, &thinking_ms, &eating_ms]() {
            auto first_fork = i%2 == 0 ? i : i+1;
            auto second_fork = i%2 == 0 ? i+1 : i;
            Philosofer philosofer{i + 1, m_forks_mutexes[first_fork], m_forks_mutexes[second_fork], thinking_ms, eating_ms};
            philosofer.live(stop_signal);
        };

        m_philosofers_threads.push_back(std::move(std::thread{lauch_philosofer}));
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    stop_signal.exchange(true);

    for(auto& thread : m_philosofers_threads) {
        thread.join();
    }

    std::cout << "Total running time " << total_running_time_s << " s" << std::endl;
}


int main(int argc, char *argv[]) 
{
    if (argc < 3)
    {
        std::cout << "Enter all required parameters " << std::endl;
        return 1;
    }
    
    std::size_t num_philosofers = atoi(argv[1]);
    std::size_t total_running_time = atoi(argv[2]);
    let_philosofers_live(num_philosofers, total_running_time, 50, 50);

    return 0;
}