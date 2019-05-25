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
#include <unordered_map>
#include <type_traits>
#include <typeinfo>

#include <boost/locale.hpp>
#include <boost/locale/boundary/segment.hpp>
#include <boost/locale/boundary/index.hpp>

#include "time_measure.h"
#include "config.h"
#include "additional.h"
#include "my_archive.h"


template<typename T>
class MyQueue{

    private:
        std::mutex mtx;
        std::condition_variable cv; // https://ru.cppreference.com/w/cpp/thread/condition_variable
        std::queue<T> *q = new std::queue<T>;
        int producers_num = 0;
        bool notified = false;

    public:
        bool empty(){
            return q->empty();
        }

        unsigned long size(){
            std::lock_guard<std::mutex> lock(mtx);
            return q->size();
        }

        void producers_number(int num){
            producers_num = num;
        }

        void finish(){
            std::lock_guard<std::mutex> lock(mtx);
            producers_num--;
            cv.notify_all();
            std::cout << "Someone finished: " << producers_num << std::endl;
        }

        void push(T element){
            std::cout << "PUSH" << std::endl;
            std::unique_lock<std::mutex> lock(mtx);
            q->push(element);
            notified = true;
            cv.notify_one();
        }

        std::vector<T> pop() {
            std::cout << "Size: " << q->size() << std::endl;
            std::cout << "POP INDEX" << std::endl;
            std::vector<T> res;
            std::unique_lock<std::mutex> lock(mtx);
            while((producers_num > 0) || !q->empty()) {
                while (!notified){
                    std::cout << "Wait INDEX" << std::endl;
                    cv.wait(lock);
                }
                while(!q->empty() ){
                    T val = q->front();
                    q->pop();
                    std::cout << "Wake up INDEX" << std::endl;
                    res.emplace_back(val);
//                std::cout << val << std::endl;
                }
                notified = false;
            }
            return res;
        }

        std::vector<T> pop_some(int n=2){
            std::cout << "Size: " << q->size() << std::endl;
            std::cout << "POP MERGE" << std::endl;
            std::vector<T> some;
            std::unique_lock<std::mutex> lock(mtx);
            while ((producers_num > 0) || (q->size() >= n)) {
                if(!notified){
                    std::cout << "Wait MERGE" << std::endl;
                    cv.wait(lock);
                }
                if(q->size() >= n) {
                    while (n > 0) {
                        std::cout << "Wake up MERGE" << std::endl;
                        some.emplace_back(q->front());
                        q->pop();
                        n--;
                    }
                }
                notified = false;
            }
            for (auto &el: some) {
                for (auto &el_1: el) {
                    std::cout << el_1.first << " " << el_1.second << std::endl;
                }
            }
            return some;
        }
};


void index_string(MyQueue<std::string> &mq_str, MyQueue< std::map<std::string, int> > &mq_map){
        std::cout << "TRY pop INDEX" << std::endl;
        auto res = mq_str.pop();

        if (!res.empty()) {
            std::string content = res[0];

            using namespace boost::locale::boundary;

            std::locale loc = boost::locale::generator().generate("en_US.UTF-8");

            // fold case
            content = boost::locale::to_lower(content, loc);

            // words to map
            std::map<std::string, int> cut_words;
            ssegment_index map(word, content.begin(), content.end(), loc);
            map.rule(word_any);

            for (ssegment_index::iterator it = map.begin(), e = map.end(); it != e; ++it) {
                cut_words[*it]++;
            }

            std::cout << "TRY push INDEX" << std::endl;
            mq_map.push(cut_words);
        }
        mq_map.finish();

    std::cout << "END INDEX THREAD" << std::endl;
}


void merge_maps(MyQueue< std::map<std::string, int> > &mq_maps){

        std::cout << "TRY pop MERGE" << std::endl;
        auto maps = mq_maps.pop_some(2);
        if (!maps.empty()){
            std::map<std::string, int> new_map;
            for (auto map: maps){
                for (auto el: map){
                    if (new_map.find(el.first) == new_map.end()){
                        new_map[el.first] = map[el.first];
                    } else{
                        new_map[el.first] += map[el.first];
                    }
                }
            }
            std::cout << "TRY push MERGE" << std::endl;
            mq_maps.push(new_map);
        }

    std::cout << "END MERGE THREAD" << std::endl;
}


int main()
{
    std::string conf_file_name = "config.dat";

    // config
    MyConfig mc;
    mc.load_configs_from_file(conf_file_name);
    if (mc.is_configured()) {
        std::cout << "YES! Configurations loaded successfully.\n" << std::endl;
    } else { std::cerr << "Error. Not all configurations were loaded properly."; return -3;}

    std::string content;

    MyQueue< std::string > mq_str;
    MyQueue< std::map<std::string, int> > mq_map;

    mq_str.producers_number(1);
    mq_map.producers_number(mc.indexing_threads);


    // opening archive
    if (is_file_ext(mc.in_file, ".zip")){
        MyArchive arc;
        if (arc.init(mc.in_file) != 0){
            return -4;
        }
        while (arc.next_content_available()){
            content = arc.get_next_content();
            mq_str.push(content);
            mq_str.finish();
        }

    } else{
        std::ifstream in_f(mc.in_file);
        if (! in_f.is_open() || in_f.rdstate())
        { std::cerr << "Couldn't open input-file."; return -2; }
        std::stringstream ss;
        ss << in_f.rdbuf();
        content = ss.str();
    }

    std::cout << "Everything had been read" << std::endl;

    std::vector<std::thread> all_my_threads;

    std::vector<std::string> ls_filenames = {"data.txt", "data1.txt", "data2.txt"};

    std::cout << "***************START******************" << std::endl;

    for (int i=0; i < mc.indexing_threads; i++){
        std::thread nt_index(&index_string, std::ref(mq_str), std::ref(mq_map));
        all_my_threads.emplace_back(std::move(nt_index));
    }

    for (int i=0; i < mc.merging_threads; i++){
        std::thread nt_merge(&merge_maps, std::ref(mq_map));
        all_my_threads.emplace_back(std::move(nt_merge));
    }

//    std::cout << "***************JOIN******************" << std::endl;

    for (auto &thr: all_my_threads)
        { thr.join(); }


    auto res = mq_map.pop();
//    std::cout << (*res.begin()).first;
    for (auto &el: res) {
        for (auto &el_1: el) {
            std::cout << el_1.first << " " << el_1.second << std::endl;
        }
    }

    return 0;
}



