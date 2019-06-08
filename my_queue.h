#ifndef COUNTING_WORDS_MY_QUEUE_H
#define COUNTING_WORDS_MY_QUEUE_H

#include <iostream>
#include <mutex>
#include <queue>


template<typename T>
class MyQueue{

private:
    std::mutex mtx;
    std::condition_variable cv; // https://ru.cppreference.com/w/cpp/thread/condition_variable
    std::queue<T> *q = new std::queue<T>;
    int producers_num = 0;
    bool notified = false;

public:
    void set_producers_number(int num){
        producers_num = num;
    }
    void finish(){
        std::lock_guard<std::mutex> lock(mtx);
        producers_num--;
        if (producers_num == 0){
        }
        notified = true;
        cv.notify_all();
    }

    void push(T element){
        std::unique_lock<std::mutex> lock(mtx);
        q->push(element);
        notified = true;
        cv.notify_one();
    }

    bool can_try_pop(int n){
        return (producers_num > 0) || (q->size() >= n);
    }

    std::vector<T> pop(int n){
        std::vector<T> some;
        std::unique_lock<std::mutex> lock(mtx);
        while ((producers_num > 0) || (q->size() >= n)) {
            while(!notified && (q->size() < n)){
                cv.wait(lock);
            }
            while(q->size() >= n) {
                while (n > 0) {
                    some.emplace_back(q->front());
                    q->pop();
                    n--;
                }
                return some;
            }
            notified = false;
        }
        return some;
    }
};

#endif //COUNTING_WORDS_MY_QUEUE_H