#ifndef COUNTING_WORDS_2_MERGE_H
#define COUNTING_WORDS_2_MERGE_H

#include <iostream>
#include <string>
#include <map>

#include "my_queue.h"


std::map<std::string, int> merge_maps(std::vector< std::map<std::string, int> > maps);

void merge_thread(MyQueue< std::map<std::string, int> > &mq_maps);


#endif //COUNTING_WORDS_2_MERGE_H
