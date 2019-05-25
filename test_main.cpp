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
        std::queue<T> *q;
    public:
        bool empty(){ return q->empty(); }

        void push(T &element){
            std::unique_lock<std::mutex> lock(mtx);
            q->emplace(element);
            cv.notify_one();
        }

        void pop(){
            std::unique_lock<std::mutex> lock(mtx);
            while(!q->empty()){
                T *element = reinterpret_cast<T*>(malloc(sizeof(T)));
                *element = q->back();
                q->pop();
                std::cout << element;
            }
        }

};


int main()
{


    return 0;
}

