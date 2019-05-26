#include "merge.h"


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
    std::vector<std::map<std::string, int>> task;
    while (mq_maps.can_try_pop(2)){
        task = mq_maps.pop(2);
        if (!task.empty()){
            mq_maps.push(merge_maps(task));
        }
    }
}
