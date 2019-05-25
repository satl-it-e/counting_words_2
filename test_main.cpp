#include <condition_variable>
#include <mutex>
#include <thread>
#include <iostream>
#include <queue>
#include <map>
#include <string>
#include <chrono>
#include <vector>
#include <fstream>

template<typename T>
class MyQueue{

    private:
        std::mutex mtx;
        std::condition_variable cv;
        std::queue<T> *q = new std::queue<T>;
        bool notified = false;                         // https://ru.cppreference.com/w/cpp/thread/condition_variable

    public:
        bool empty(){ return q->empty(); }

        void push(T element){
            std::unique_lock<std::mutex> lock(mtx);
            q->push(element);
            notified = true;
            cv.notify_one();
        }

        void pop(){
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this]() { return notified;});
            while(!q->empty()){
                T *element = reinterpret_cast<T*>(malloc(sizeof(T)));
                *element = q->back();
                q->pop();
                std::cout << element;
            }
            notified = false;
        }

};


int main()
{
    MyQueue<std::string> mq;
    mq.push("qwerty");

    return 0;
}

