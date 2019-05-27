#include "my_queue.h"


void MyQueue::set_producers_number(int num){
    producers_num = num;
}


void MyQueue::finish(){
    std::lock_guard<std::mutex> lock(mtx);
    producers_num--;
    if (producers_num == 0){
    }
    notified = true;
    cv.notify_all();
}


void MyQueue::push(T element){
    std::unique_lock<std::mutex> lock(mtx);
    q->push(element);
    notified = true;
    cv.notify_all();
}


bool MyQueue::can_try_pop(int n){
    return (producers_num > 0) || (q->size() >= n);
}


std::vector<T> MyQueue::pop(int n){
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