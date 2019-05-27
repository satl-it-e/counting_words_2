
template<typename T >
class MyQueue{

private:
    std::mutex mtx;
    std::condition_variable cv; // https://ru.cppreference.com/w/cpp/thread/condition_variable
    std::queue<T> *q = new std::queue<T>;
    int producers_num = 0;
    bool notified = false;

public:
    void set_producers_number(int num);
    void finish();
    void push(T element);
    bool can_try_pop(int n);
    std::vector<T> pop(int n);
};