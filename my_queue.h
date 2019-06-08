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
    std::condition_variable cv_producer;
    std::queue<T> *q = new std::queue<T>;
    unsigned int limit = 20;
    int producers_num = 0;
    bool notified = false;
    bool notified_producer = false;


public:
    void set_queue_limit(unsigned int lim){
        limit = lim;
    }

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

    void force_push(T element){
        std::unique_lock<std::mutex> lock(mtx);
        q->push(element);
        notified = true;
        cv.notify_one();
    }

    void push(T element){
        std::unique_lock<std::mutex> lock(mtx);
        while((!notified_producer) && (q->size() >= limit)){
            cv_producer.wait(lock);
        }
        if(q->size() < limit){
            q->push(element);
            notified = true;
            cv.notify_one();
        }
        notified_producer = false;
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
                    cv_producer.notify_one();
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