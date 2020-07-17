#include <thread>
#include <queue>
#include <mutex>
#include <chrono>
#include <iostream>

using namespace std;
int questionableLogs = 0;
mutex questionableMutex;
    
template <class T> class Pool
{
    public:
        Pool(size_t maxSize = 100)
            : mMaxSize(maxSize) {}
        
        ~Pool() {
            mMutex.lock();
            while (mContainer.size()) {
                mContainer.pop();
            }
            mMutex.unlock();
        }

        size_t size() {
            mMutex.lock();
            size_t result = mContainer.size();
            mMutex.unlock();
            return result;
        }
        
        void push(T item) {
            mMutex.lock();
            while (mContainer.size() >= mMaxSize) {
                mMutex.unlock();
                this_thread::sleep_for(chrono::milliseconds(10));
                mMutex.lock();
            }
            mContainer.push(item);
            mMutex.unlock();
        }

        T pop() {
            mMutex.lock();
            int counter = 0;
            while (mContainer.size() <= 0 && counter < 10) {
                mMutex.unlock();
                this_thread::sleep_for(chrono::milliseconds(10));
                mMutex.lock();
                ++counter;
            }
            if (counter == 10) {
                return T();
            }
            T item = mContainer.front(); 
            mContainer.pop();
            mMutex.unlock();
            return item;
        }

    private:
        size_t mMaxSize;
        queue<T> mContainer;
        mutex mMutex;
};

class Producer
{
    public:
        Producer(Pool<vector<string>> *pool, vector<string> routerLog)
            : mPool(pool), 
              mRouterLog(routerLog) {}
        virtual void run() {
            if (!mPool) 
                return;
            // find device in first line
            if (mRouterLog[0].find("device:") != 0)
                return;
            if (mRouterLog[1].find("url:") != 0)
                return;
            if (mRouterLog[2].find("timestamp:") != 0)
                return;
            // input is OK
            mPool->push(mRouterLog);
        }
    private:
        Pool<vector<string>> *mPool;
        vector<string> mRouterLog;
};

class Consumer
{
    public:
        Consumer(Pool<vector<string>> *pool, const vector<string> &offendingWords)
            : mPool(pool), 
              mOffendingWords(offendingWords) {}
        virtual void run() {
            // FIXME: trying to consume all the time items left in pool by producer
            //chrono::system_clock::duration begin = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch());
            //while (true) {
                if (!mPool) {
                    return;
                }
                vector<string> log = mPool->pop();
                if (log.empty()) {
                    return;
                }
                bool questionable = isQuestionable(log);
                if (questionable) {
                    // sleep 50ms and store data
                    storeQuestionableLog();
                    this_thread::sleep_for(chrono::milliseconds(50));
                }
                //chrono::system_clock::duration end = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch());
                //if (end - begin > chrono::milliseconds(200)) {
                //    break;
                //}
            //}
        }
    private:
        static mutex mMutex;
        Pool<vector<string>> *mPool;
        vector<string> mOffendingWords;
        bool isQuestionable(const vector<string> &log) {
            // verify offending words
            for (auto word : mOffendingWords) {
                if (log[1].find(word) != string::npos) {
                    return true;
                }
            }
            return false;
        }
        void storeQuestionableLog() {
            questionableMutex.lock();
            ++questionableLogs;
            questionableMutex.unlock();
        }
};

vector<string> input;
vector<string> offendingWords{"porn", "sex", "xxx", "Bieber"};
Pool<vector<string>> *pool = new Pool<vector<string>>;

void run_thread_producer(vector<string> input) {
    Producer *producer = new Producer(pool, input);
    producer->run();
}

void run_thread_consumer() {
    Consumer *consumer = new Consumer(pool, offendingWords);
    consumer->run();
}

int main() {
    /* Enter your code here. Read input from STDIN. Print output to STDOUT */
    int numberOfConsumers = 3;
    vector<thread *> consumers;
    for (int i = 0; i < numberOfConsumers; ++i) {
        consumers.push_back(new thread(run_thread_consumer));
    }
    while (!cin.eof()) {
        string line;
        getline(cin, line);
        input.push_back(line);
        if (input.size() == 3) {
            // produce
            thread myProducer(run_thread_producer, input);
            myProducer.join();
            input.clear();
        }
    }
    for (std::thread *threadToJoin : consumers) {
        if (threadToJoin->joinable()) {
            threadToJoin->join();
	}
	delete threadToJoin;
    }
    cout << questionableLogs;
    return 0;
}
