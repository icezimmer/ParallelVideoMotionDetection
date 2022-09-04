class threadPool {

private:
    queue<function<void(void)>> tasks; // by packaged task we can get a future
    //vector<future<void>> vf; // future of the tasks
    vector<thread> threads; // vector of threads (nm. of threads = size)
    mutex m;
    condition_variable c;
    int size;
    bool stop;

    void body() {
        while(true) {
            function<void(void)> tsk; //initialization
            {
                unique_lock<mutex> l(this->m);
                this->c.wait(l, [&](){ return (!this->tasks.empty() || this->stop); }); // wait until queue is not empty or the threadpool is stopped
                if(!this->tasks.empty()){
                    tsk = this->tasks.front();
                    this->tasks.pop();
                }
                if(this->stop) return;    
            }
            tsk();
        }
    }

public:

    void stopThreadPool() {
        {
            unique_lock<mutex> l(this->m);
            this->stop = true;
        }
        this->c.notify_all();
    }

    threadPool(int size) {
        this->size = size;
        this->stop = false;

        for(int worker=0; worker<this->size; worker++)
            this->threads.emplace_back([this](){ this->body(); });
    }

    ~threadPool() {
        for(auto& thread : this->threads)
            thread.join();
    }

    void submit(function<void(void)> fx) {
        {
            unique_lock<mutex> l(this->m);
            this->tasks.push(fx);
        }
        this->c.notify_one();
    }
  
};


class NativeParallel {

    private:
        VideoCapture* source;     // Source of video
        int width,height;         // Shape of frame
        int totalf;               // Number of total frame in the video
        VideoDetection* vd;
        Mat* background;          // Background images used for comparisons
        atomic<ulong> totalDiff;  // Variable used to accomulate frame "detected"
        atomic<int> toCompute;    // Variable representing the number of task to compute
        int nw;                   // Number of workers

        void cleanUp() {
            source->release();
            delete background;
            delete source;
            delete vd;
        }

    public:
        NativeParallel(const string path,const float k,const int nw):
            totalDiff(0),nw(nw) {

            // checking argument
            ERROR(path == "","path error")
            ERROR(k<= 0 || k>1,"%'of pixel must be between 0 and 1")
            ERROR(nw<= 0,"Workers must be more than 0")

            this->source = new VideoCapture(path); 

            // Check if the video is opened
            ERROR(!source->isOpened(),"Error opening video")

            this->width  = source->get(CAP_PROP_FRAME_WIDTH);
            this->height = source->get(CAP_PROP_FRAME_HEIGHT);
            this->totalf = source->get(CAP_PROP_FRAME_COUNT);
            this->toCompute = totalf-1; 

            // We need at least 2 frame (one is the background)
            ERROR(totalf<2,"Too short video")

            this->vd = new VideoDetection(width,height,k);

            // We retrieve the background ----
                Mat frame;
                Mat *grey_pad = new Mat(height+1,width+1,CV_8UC1,BLACK); // Auxiliar memory frame
                this->background = new Mat(height,width,CV_8UC1,BLACK);       
                ERROR(!source->read(frame),"Error in read frame operation") // Take the fist frame of the video
                vd->RGBtoGrey_pad(frame,grey_pad);
                vd->smoothing_pad(grey_pad,this->background);
                vd->setBackground(this->background);
                delete grey_pad;
        }
        
        void run() {

            {
                threadPool tp(nw);

                function<void(Mat)> work = [&] (Mat frame) {
                    totalDiff += vd->composition_pad(frame);
                    if(--toCompute == 0)
                        tp.stopThreadPool();
                    return;
                };

                auto frame_streaming = [&] (int totalf) {
                    for(int f=0;f<totalf-1;f++){
                        Mat frame;
                        ERROR(!source->read(frame),"Error in read frame operation")
                        auto work_frame = bind(work,frame);
                        tp.submit(work_frame); //submit the task
                    }
                };

            thread tid_str(frame_streaming, totalf);
            tid_str.join();

            }

            cout << "Total frame: " << totalf << endl;
            cout << "Total diff: " << totalDiff << endl;
            cleanUp();
            exit(0);

        }


        void time() {

            long elapsed;
            {   
                utimer u("NativeParallel",&elapsed);
                threadPool tp(nw);

                function<void(Mat)> work = [&] (Mat frame) {
                    totalDiff += vd->composition_pad(frame);
                    if(--toCompute == 0)
                        tp.stopThreadPool();
                    return;
                };

                auto frame_streaming = [&] (int totalf) {
                    for(int f=0;f<totalf-1;f++){
                        Mat frame;
                        ERROR(!source->read(frame),"Error in read frame operation")
                        auto work_frame = bind(work,frame);
                        tp.submit(work_frame); //submit the task
                    }
                };

            thread tid_str(frame_streaming, totalf);
            tid_str.join();

            }

            cleanUp();
            exit(0); 
        }

        void overhead() {

            long elapsed;
            {   
                utimer u("OVERHEAD [NativeParallel]",&elapsed);
                threadPool tp(nw);

                function<void(Mat)> work = [&] (Mat frame) {
                    if(--toCompute == 0)
                        tp.stopThreadPool();
                    return;
                };

                auto frame_streaming = [&] (int totalf) {
                    for(int f=0;f<totalf-1;f++){
                        Mat frame;
                        auto work_frame = bind(work,frame);
                        tp.submit(work_frame);
                    }
                };

            thread tid_str(frame_streaming, totalf);
            tid_str.join();

            }

            cleanUp();
            exit(0); 
        }
};