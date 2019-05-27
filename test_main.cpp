#include <condition_variable>
#include <mutex>
#include <thread>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <map>
#include <string>
#include <chrono>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <type_traits>
#include <typeinfo>
#include <queue>

#include <boost/locale.hpp>
#include <boost/locale/boundary/segment.hpp>
#include <boost/locale/boundary/index.hpp>

#include "time_measure.h"
#include "config.h"
#include "additional.h"
#include "my_archive.h"
//#include "my_queue.h"
//#include "index.h"
//#include "merge.h"

template<typename T >
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
            std::cout << "There are no producers here" << std::endl;
        }
        std::cout << "NOTIFICATION FINISH 🤡" << std::endl;
        notified = true;
        cv.notify_all();
    }

    void push(T element){
        std::unique_lock<std::mutex> lock(mtx);
        q->push(element);
        notified = true;
        std::cout << "NOTIFICATION PUSH 🐼" << std::endl;
        cv.notify_all();
    }

    bool can_try_pop(int n){
        return (producers_num > 0) || (q->size() >= n);
    }

    std::vector<T> pop(int n){
        std::vector<T> some;
        std::unique_lock<std::mutex> lock(mtx);
        while ((producers_num > 0) || (q->size() >= n)) {
            while(!notified && (q->size() < n)){
                std::cout << "Someone gone sleep." << std::endl;
                cv.wait(lock);
            }
            while(q->size() >= n) {
                std::cout << "I'm here!" << std::endl;
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

using namespace boost::locale::boundary;


std::map<std::string, int> index_string(std::string content){
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

    return cut_words;
}

void index_thread(MyQueue<std::string> &mq_str, MyQueue< std::map<std::string, int>> &mq_map){
    std::cout << "INDEX🐠 thread started" << std::endl;
    std::vector<std::string> task;
    while (mq_str.can_try_pop(1)){
        std::cout << "INDEX🐠 pop started🥏" << std::endl;
        task = mq_str.pop(1);
        for (std::string &part: task){
            std::cout << "INDEX🐠 pop SUCCESS🍏 and pushed" << std::endl;
            mq_map.push(index_string(part));
        }
        if (task.empty()){
            std::cout << "INDEX🐠 pop FAIL🍎" << std::endl;
        }
    }
    std::cout << "INDEX🐠 thread FINISHED" << std::endl;
    mq_map.finish();
}


std::map<std::string, int> merge_maps(std::vector< std::map<std::string, int> > maps){
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
    return new_map;
}


void merge_thread(MyQueue< std::map<std::string, int> > &mq_maps){
    std::cout << "MERGE🦄 thread started" << std::endl;
    std::vector<std::map<std::string, int>> task;
    while (mq_maps.can_try_pop(2)){
        std::cout << "MERGE🦄 pop started🥏" << std::endl;
        task = mq_maps.pop(2);
        if (!task.empty()){
            std::cout << "MERGE🦄 pop SUCCESS🍏 and pushing" << std::endl;
            mq_maps.push(merge_maps(task));
        } else{
            std::cout << "MERGE🦄 pop FAIL🍎" << std::endl;
        }
    }
    std::cout << "MERGE🦄 thread FINISHED" << std::endl;
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

    mq_str.set_producers_number(1);
    mq_map.set_producers_number(mc.indexing_threads);

    auto gen_st_time = get_current_time_fenced();

    std::vector<std::thread> all_my_threads;

    for (int i=0; i < mc.indexing_threads; i++){
        all_my_threads.emplace_back(index_thread, std::ref(mq_str), std::ref(mq_map));
    }

    for (int i=0; i < mc.merging_threads; i++){
        all_my_threads.emplace_back(merge_thread, std::ref(mq_map));
    }


    // opening archive
    if (is_file_ext(mc.in_file, ".zip")){
        MyArchive arc;
        if (arc.init(mc.in_file) != 0){
            return -4;
        }
        while (arc.next_content_available()){
            content = arc.get_next_content();
            std::cout << "Reader: " << std::endl;
            mq_str.push(content);
        }
        mq_str.finish();

    } else{
        std::ifstream in_f(mc.in_file);
        if (! in_f.is_open() || in_f.rdstate())
        { std::cerr << "Couldn't open input-file."; return -2; }
        std::stringstream ss;
        ss << in_f.rdbuf();
        content = ss.str();
        mq_str.push(content);
        mq_str.finish();
    }

    for (auto &thr: all_my_threads)
    { thr.join(); }

    auto read_fn_time = get_current_time_fenced();

    std::cout << "Everything had been READ from archive." << std::endl;

    std::cout << "***************START******************" << std::endl;



    auto index_fn_time = get_current_time_fenced();


    std::cout << "********Trying to get result**********" << std::endl;


    if (!mq_map.can_try_pop(1)) {
        std::cerr << "Error in MAP QUEUE" << std::endl;
        return -6;
    }

    std::cout << "QUEUE is NOT empty" << std::endl;
    auto res = mq_map.pop(1).front();
    std::cout << "POP successfull" << std::endl;

    std::vector<std::pair<std::string, int> > vector_words;
    for (auto &word: res){
        vector_words.emplace_back(word);
    }

    std::sort(vector_words.begin(), vector_words.end(), [](const auto t1, const auto t2){ return t1.second < t2.second;});
    std::ofstream num_out_f(mc.to_numb_file);
    for (auto &v : vector_words) {
        num_out_f << std::left << std::setw(20) << v.first << ": ";
        num_out_f << std::right << std::setw(10) << std::to_string(v.second) << std::endl;
    }

    std::sort(vector_words.begin(), vector_words.end(), [](const auto t1, const auto t2){ return t1.first.compare(t2.first)<0;});
    std::ofstream alp_out_f(mc.to_alph_file);
    for (auto &v : vector_words) {
        alp_out_f << std::left << std::setw(20) << v.first <<  ": ";
        alp_out_f << std::right << std::setw(10) << std::to_string(v.second) << std::endl;
    }

    auto gen_fn_time = get_current_time_fenced();         //~~~~~~~~~ general finish


    std::cout << std::left  << std::setw(35) <<  "General time (read-index-write): ";
    std::cout << std::right  << std::setw(10) << to_us(gen_fn_time - gen_st_time) << std::endl;
    std::cout << std::left  << std::setw(35) << "Reading time: ";
    std::cout << std::right << std::setw(10) << to_us(read_fn_time - gen_st_time)  << std::endl;
    std::cout << std::left << std::setw(35) << "Indexing time (boost included): " ;
    std::cout << std::right  << std::setw(10) << to_us(index_fn_time - read_fn_time)  << std::endl;

    std::cout << "\nFinished.\n" << std::endl;


    return 0;
}



