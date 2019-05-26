#include "my_queue.h"

template<typename T >
void MyQueue<T>::set_producers_number(int num){
    producers_num = num;
}


template<typename T >
void MyQueue<T>::finish(){
        std::lock_guard<std::mutex> lock(mtx);
        producers_num--;
        cv.notify_all();
    }

template<typename T >
void MyQueue<T>::push(T element){
        std::unique_lock<std::mutex> lock(mtx);
        q->push(element);
        notified = true;
        cv.notify_all();
    }

template<typename T >
bool MyQueue<T>::can_try_pop(int n){
    return (producers_num > 0) || (q->size() >= n);
}

template<typename T >
std::vector<T> MyQueue<T>::pop(int n){
    std::vector<T> some;
    std::unique_lock<std::mutex> lock(mtx);
    while ((producers_num > 0) || (q->size() >= n)) {
        while(!notified){
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

