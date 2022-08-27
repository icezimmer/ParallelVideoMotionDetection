class Task {
    public:
        Mat frame;
        ushort detected;

        Task():detected(0) {}
};

struct Emitter: ff_node_t<Task> {
    VideoCapture* source;
    const int totalf;

    Emitter(VideoCapture* source, const int totalf):source(source),totalf(totalf) {}

    Task* svc(Task *) {

        for(int f=0; f<totalf-1; f++) {
            Task* tsk = new Task();
            ERROR(!source->read(tsk->frame),"Error in read frame operation")
            ff_send_out(tsk);
        }
        return EOS;
    }

    // void svc_end() { 
    //     cout << "Total frame: " << totalf << endl;
    // }

};

struct Worker: ff_node_t<Task> {
    VideoDetection* vd;

    Worker(VideoDetection* vd):vd(vd) {}

    Task* svc(Task * tsk) { 
        tsk->detected = vd->composition(tsk->frame);
        return tsk; 
    }
};

struct Collector: ff_node_t<Task> {
    ulong* totalDiff;

    Collector(ulong* totalDiff):totalDiff(totalDiff) {}

    Task* svc(Task * tsk) { 
        *totalDiff += tsk->detected;
        delete tsk;
        return GO_ON; 
    }

    // void svc_end() { 
    //     cout << "Total diff: " << totalDiff << endl;
    //     }
};


class FastFlow {
    private:
    VideoCapture* source;  // Source of video
    int width,height;      // Shapes of frame
    int totalf;            // Number of total frame in the video
    VideoDetection* vd;
    Mat* background;       // Background images used for comparisons
    ulong totalDiff;  // Variable used to accomulate frame "detected"
    int nw;                // Farm-Worker(Farm)

    void cleanUp() {
        source->release();
        delete background;
        delete source;
    }
    public:
    FastFlow(const string path,const float k,const int nw):
        totalDiff(0),nw(nw) { 

        // checking argument
        ERROR(path == "","path error")
        ERROR(k<= 0 || k>1,"%'of pixel must be between 0 and 1")
        ERROR(nw<= 0,"Workers must be more than 0")

        this->source = new VideoCapture(path);

        // check if the video is opened
        ERROR(!source->isOpened(),"Error opening video")

        this->width  = source->get(CAP_PROP_FRAME_WIDTH);
        this->height = source->get(CAP_PROP_FRAME_HEIGHT);
        this->totalf = source->get(CAP_PROP_FRAME_COUNT);

        // We need at least 2 frame: one is the background, the other is the frame to compare
        ERROR(totalf<3,"Too short video")
  
        this->vd = new VideoDetection(width,height,k);

        // ---- First of all we retrieve the background ----
            Mat frame;        
            this->background = new Mat(height,width,CV_8UC1,BLACK);
            // take the fist frame of the video
            ERROR(!source->read(frame),"Error in read frame operation")
            vd->RGBtoGray(frame,this->background);
            vd->smoothing(this->background);
            vd->setBackground(this->background);
    }

    void execute_to_result() {

        Emitter  emitter(source, totalf);
        Worker wrk(vd);
        Collector  collector(&totalDiff);

        ff_Farm<Task> farm(
            [&]() {
                vector<unique_ptr<ff_node> > W;
                for(int i=0;i<nw-2;++i)
                    W.push_back(make_unique<Worker>(wrk));
                return W;
            } ()
        );

        ff_Pipe pipe(emitter, farm, collector);

        if (pipe.run_and_wait_end()<0) {
            error("running pipe");
            exit(1);
        }

        cout << "Total frame: " << totalf << endl;
        cout << "Total diff: " << totalDiff << endl;
        cleanUp();
        exit(0);
    }

    void execute_to_stat() {

        long elapsed;
        {
            utimer u("FastFlow",&elapsed);
            Emitter  emitter(source, totalf);
            Worker wrk(vd);
            Collector  collector(&totalDiff);

            ff_Farm<Task> farm(
                [&]() {
                    vector<unique_ptr<ff_node> > W;
                    for(int i=0;i<nw-2;++i)
                        W.push_back(make_unique<Worker>(wrk));
                    return W;
                } ()
            );

            ff_Pipe pipe(emitter, farm, collector);

            if (pipe.run_and_wait_end()<0) {
                error("running pipe");
                exit(1);
            }
        }

        cleanUp();
        exit(0);
    }
};