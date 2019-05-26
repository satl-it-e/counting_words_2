#ifndef COUNTING_WORDS_2_INDEX_H
#define COUNTING_WORDS_2_INDEX_H

#include <iostream>
#include <string>
#include <map>

#include <boost/locale.hpp>
#include <boost/locale/boundary/segment.hpp>
#include <boost/locale/boundary/index.hpp>

#include "my_queue.h"


std::map<std::string, int> index_string(std::string content);

void index_thread(MyQueue<std::string> &mq_str, MyQueue< std::map<std::string, int>> &mq_map);


#endif //COUNTING_WORDS_2_INDEX_H
