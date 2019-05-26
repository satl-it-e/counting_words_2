#include "index.h"


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
    std::vector<std::string> task;
    while (mq_str.can_try_pop(1)){
        task = mq_str.pop(1);
        for (std::string &part: task){
            mq_map.push(index_string(part));
        }
    }
    mq_map.finish();
}
