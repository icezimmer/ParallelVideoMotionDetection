/**
 * @brief The shared queue is used to hide a lock and mutex mechanism and to provide 
 * a mutal-exclusion queue.
 * 
 */
class SQueue {
    
    private:
        queue<Mat*> frameQ; // Queue of frames
        mutex mtx; // Mutex
        condition_variable c; 
    public:
        atomic<bool> finished; // True if all frame are red

        SQueue(): finished(false) { }

        void end() { finished = true; c.notify_all(); }

        // Load a new frame in queue
        void push(Mat* v) { 
            unique_lock<mutex> l(mtx);
            frameQ.push(v);
            c.notify_one();
        }
        // Retrieve a frame
        Mat* get()  {
            unique_lock<mutex> l(mtx);
            c.wait(l,[&]{return !frameQ.empty() || finished.load();});
        
            if(!frameQ.empty()) {
                Mat* ret = frameQ.front();
                frameQ.pop();
                return ret;
            }
            return nullptr; // fro indicate that there are no other frame
        }
};

void active_delay(int msecs) {
    // read current time
    auto start = std::chrono::high_resolution_clock::now();
    auto end   = false;
    while(!end) {
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        if(msec>msecs)
            end = true;
    }
    return;
}