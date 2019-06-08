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

#include <boost/locale.hpp>
#include <boost/locale/boundary/segment.hpp>
#include <boost/locale/boundary/index.hpp>
#include <boost/filesystem.hpp>

#include "time_measure.h"
#include "config.h"
#include "additional.h"
#include "my_archive.h"
#include "my_queue.h"



using namespace boost::locale::boundary;
using namespace boost::filesystem;


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
    std::vector<std::string> task;
    while (mq_str.can_try_pop(1)){
        task = mq_str.pop(1);
        for (std::string &part: task){
            mq_map.push(index_string(part));
        }
        if (task.empty()){
        }
    }
    mq_map.finish();
}


std::map<std::string, int> merge_maps(std::vector< std::map<std::string, int> > maps){
    std::map<std::string, int> new_map;
    for (auto &map: maps){
        for (auto &el: map){
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
    std::vector<std::map<std::string, int>> task;
    while (mq_maps.can_try_pop(2)){
        task = mq_maps.pop(2);
        if (!task.empty()){
            mq_maps.push(merge_maps(task));
        } else{
        }
    }
}


int read_entry(MyArchive &marc, MyQueue<std::string> &mq_str, path x){
    //    // opening archive
    std::string filename = x.string(), content;
    if (is_file_ext(filename, ".zip")){
        if (marc.init(filename) != 0){
            std::cerr << "Something wrong with file: " << filename << std::endl;
            return -1;
        }
        while (marc.next_content_available()){
            content = marc.get_next_content();
            mq_str.push(content);
        }

    } else{
        std::ifstream in_f(filename);
        if (! in_f.is_open() || in_f.rdstate()){
            std::cerr << "Couldn't open the file. " << filename << std::endl;
            return -2;
        }
        std::stringstream ss;
        ss << in_f.rdbuf();
        content = ss.str();
        mq_str.push(content);
    }
}



void process_deep(MyArchive &marc, MyQueue<std::string> &mq_str, path x){
    std::cout << "\t" << x << std::endl;
    if (is_regular_file(x)){
        read_entry(marc, mq_str, x);
    }

    else if (is_directory(x))
    {
//        int i = 0;
        for (directory_entry& y : directory_iterator(x)){
            process_deep(marc, mq_str, y.path());
//            i++;
//            if (i > 2){
//                break;
//            }
        }
    }

    else {
        std::cerr << x << " exists, but is not a regular file or directory\n";

    }
}


int main()
{
    std::string conf_file_name = "config.dat";

    // config
    MyConfig mc;
    mc.load_configs_from_file(conf_file_name);
    if (mc.is_configured()) {
        std::cout << "YES! Configurations loaded successfully.\n" << std::endl;
    } else { std::cerr << "Error. Not all configurations were loaded properly."; return -1;}

    std::string content;

    MyQueue< std::string > mq_str;
    MyQueue< std::map<std::string, int> > mq_map;

    mq_str.set_producers_number(1);
    mq_map.set_producers_number(mc.indexing_threads);


    std::vector<std::thread> all_my_threads;
    all_my_threads.reserve(mc.indexing_threads + mc.merging_threads);

    std::cout << "Creating threads." << std::endl;

    auto gen_st_time = get_current_time_fenced();

    for (int i=0; i < mc.indexing_threads; i++){
        all_my_threads.emplace_back(index_thread, std::ref(mq_str), std::ref(mq_map));
    }

    for (int i=0; i < mc.merging_threads; i++){
        all_my_threads.emplace_back(merge_thread, std::ref(mq_map));
    }

    // Reading the content of mc.in_file
    MyArchive my_arc;

    try
    {
        if (exists(mc.in_file))
        {
            std::cout << "Start reading." << std::endl;
            process_deep(my_arc, mq_str, mc.in_file);
            mq_str.finish();

        }
        else {
            std::cerr << mc.in_file << " does not exist\n";
            return -3;
        }
    }

    catch (const filesystem_error& ex)
    {
        std::cerr << ex.what() << '\n';
        return -2;
    }

    std::cout << "Everything had been READ from archive." << std::endl;

    for (auto &thr: all_my_threads)
    { thr.join(); }

    auto gen_fn_time = get_current_time_fenced();         //~~~~~~~~~ general finish


    if (!mq_map.can_try_pop(1)) {
        std::cerr << "Error in MAP QUEUE" << std::endl;
        return -4;
    }

    auto res = mq_map.pop(1).front();

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

    std::cout << " General time: " << to_us(gen_fn_time - gen_st_time) << std::endl;
//    std::cout << std::left  << std::setw(35) <<  "General time (read-index-write): ";
//    std::cout << std::right  << std::setw(10) << to_us(gen_fn_time - gen_st_time) << std::endl;
//    std::cout << std::left  << std::setw(35) << "Reading time: ";
//    std::cout << std::right << std::setw(10) << to_us(read_fn_time - gen_st_time)  << std::endl;
//    std::cout << std::left << std::setw(35) << "Indexing time (boost included): " ;
//    std::cout << std::right  << std::setw(10) << to_us(index_fn_time - read_fn_time)  << std::endl;

    std::cout << "\nFinished.\n" << std::endl;


    return 0;
}



