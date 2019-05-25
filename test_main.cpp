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
#include <sstream>
#include <bits/stdc++.h>
#include <boost/algorithm/string.hpp>

template<typename T>
class MyQueue{

    private:
        std::mutex mtx;
        std::condition_variable cv;
        std::queue<T> *q = new std::queue<T>;
        bool notified = false;                         // https://ru.cppreference.com/w/cpp/thread/condition_variable

    public:
        bool empty(){
            return q->empty();
        }

        int size(){
            std::lock_guard<std::mutex> lock(mtx);
            return q->size();
        }

        void push(T element){
            std::unique_lock<std::mutex> lock(mtx);
            q->push(element);
            notified = true;
            cv.notify_one();
        }

        T pop(){
            std::unique_lock<std::mutex> lock(mtx);
            while(q->empty())
                { cv.wait(lock); }
            T val = q->front();
            q->pop();
            return val;
        }

        std::vector<T> pop_some(int n=2){
            std::vector<T> some;
            std::unique_lock<std::mutex> lock(mtx);
            while (q->size() < n)
                { cv.wait(lock); }
            while (n > 0){
                some.emplace_back(q->front());
                q->pop();
                n--;
            }
            return some;
        }
};


void index_string(MyQueue<std::string> &mq_str, MyQueue< std::map<std::string, int> > &mq_map){
    auto content = mq_str.pop();
    std::vector<std::string> spl_words;
//    boost::split(spl_words, content, boost::is_any_of("\t"));

    std::map<std::string, int> new_map;
    for(auto &word : spl_words){
        new_map[word]++;
    }
    mq_map.push(new_map);
}


void merge_maps(MyQueue< std::map<std::string, int> > &mq_maps){
    while (mq_maps.size() > 1){
        auto maps = mq_maps.pop_some(2);
        for (auto &k: maps[0]){
            std::cout << typeid(maps[0]).name() << std::endl;
        }
        mq_maps.push(maps[1]);
    }
}


int main()
{
    MyQueue< std::string > mq_str;
    MyQueue< std::map<std::string, int> > mq_map;

    std::vector<std::thread> all_my_threads;

    std::vector<std::string> ls_filenames = {"data.txt", "data1.txt", "data2.txt"};

    for(auto &f_name : ls_filenames){
        std::ifstream in_f(f_name);

        std::string line;
        while (getline(in_f, line)){
            std::string content;
            int c = 0;
            for(int i = 0; i < line.length(); i++) {
                if (isspace(line[i])){
                    if (c!= i){
                        content.append(line.substr(c, i-c)+" ");
                    }
                    c = i+1;
                }
            }
            if (c != line.length()){ content.append(line.substr(c, line.length()-c)+ " "); }
        mq_str.push(std::ref(content));

//        if (! in_f)
//            { std::cerr << "Couldn't open input-file."; return -2; }
//        std::string content; std::stringstream ss;
//        ss << in_f.rdbuf();
//        content = ss.str();
    }

        std::thread nt_index(&index_string, std::ref(mq_str), std::ref(mq_map));
        std::thread nt_merge(&merge_maps, std::ref(mq_map));

        all_my_threads.emplace_back(std::move(nt_index));
        all_my_threads.emplace_back(std::move(nt_merge));


    for (auto &thr: all_my_threads)
        { thr.join(); }


    auto res = mq_map.pop();
    std::cout << (*res.begin()).first;

    return 0;
}



