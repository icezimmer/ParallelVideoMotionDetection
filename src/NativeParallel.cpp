class threadPool {

private:
    queue<packaged_task<void(void)>> tasks; // by packaged task we can get a future
    vector<future<void>> vf; // future of the tasks
    vector<thread> threads; // vector of threads (nm. of threads = size)
    mutex m;
    condition_variable c;
    int size;
    bool stop;

    void body() {
        while(true) {
            packaged_task<void(void)> tsk; //initialization
            {
                unique_lock<mutex> l(this->m);
                this->c.wait(l, [&](){ return (!this->tasks.empty() || this->stop); }); // wait until queue is not empty or the threadpool is stopped
                if(!this->tasks.empty()){
                    tsk = move(this->tasks.front());
                    this->tasks.pop();
                }
                if(this->stop) return;    
            }
            tsk();
        }
    }

    void stopThreadPool() {
        {
            unique_lock<mutex> l(this->m);
            this->stop = true;
        }
        this->c.notify_all();
    }

public:

    threadPool(int size) {
        this->size = size;
        this->stop = false;
        // i task vengono assegnati al thread pool nel costruttore affinché siano eseguibili appena entrano nella coda
        for(int worker=0; worker<this->size; worker++)
            this->threads.emplace_back([this](){ this->body(); });
    }

    ~threadPool() {
        //in this way i'm sure that all the result has been computed before that stop is setted to true
        for(ulong i=0;i<vf.size();i++)
            this->vf[i].get();

        stopThreadPool();

        for(auto& thread : this->threads)
            thread.join();
    }

    void submit(function<void(void)> fx) {
        {
            unique_lock<mutex> l(this->m);
            packaged_task<void(void)> pt(fx);
            this->vf.push_back(pt.get_future());
            this->tasks.push(move(pt));
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
        ulong totalDiff;  // Variable used to accomulate frame "detected"
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

            // We need at least 2 frame: one is the background, the other is the frame to compare
            ERROR(totalf<3,"Too short video")

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
            // As before but in this case the measure the time execution

            long elapsed;
            {   
                utimer u("NativeParallel",&elapsed);
                threadPool tp(nw);

                function<void(Mat)> work = [&] (Mat frame) { 
                        totalDiff += vd->composition_pad(frame);
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
            // As before but in this case the measure the time execution

            long elapsed;
            {   
                utimer u("OVERHEAD [NativeParallel]",&elapsed);
                threadPool tp(nw);

                function<void(Mat)> work = [] (Mat frame) { 
                        return;
                    };

                auto frame_streaming = [&] (int totalf) {
                    for(int f=0;f<totalf-1;f++){
                        Mat frame;
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
};