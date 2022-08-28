// class Task {
//     public:
//         Mat frame;
//         ushort detected;

//         Task():detected(0) {}
// };

struct EmitterCollector: ff_node_t<Task> {
    VideoCapture* source;
    const int totalf;
    int n_tasks;
    ulong* totalDiff;

    EmitterCollector(VideoCapture* source, const int totalf, ulong* totalDiff):
        source(source),totalf(totalf),n_tasks(0),totalDiff(totalDiff) {}

    Task* svc(Task *tsk) {
        if (tsk == nullptr) {
            for(int f=0; f<totalf-1; f++) {
                Task* tsk = new Task();
                ERROR(!source->read(tsk->frame),"Error in read frame operation")
                ff_send_out(tsk);
            }
            return GO_ON;
        }        
        *totalDiff += tsk->detected;
        delete tsk;
        if (++n_tasks == totalf-1) return EOS;
        return GO_ON;         
    }
};

// struct Worker: ff_node_t<Task> {
//     VideoDetection* vd;

//     Worker(VideoDetection* vd):vd(vd) {}

//     Task* svc(Task * tsk) { 
//         tsk->detected = vd->composition(tsk->frame);
//         return tsk; 
//     }
// };


class MasterWorker {
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
    MasterWorker(const string path,const float k,const int nw):
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

        // We retrieve the background ----
            Mat frame;
            Mat *aux = new Mat(height,width,CV_8UC1,BLACK); // Auxiliar memory frame
            this->background = new Mat(height,width,CV_8UC1,BLACK);       
            ERROR(!source->read(frame),"Error in read frame operation") // Take the fist frame of the video
            vd->RGBtoGray(frame,aux);
            vd->smoothing(aux,this->background);
            vd->setBackground(this->background);
            delete aux;
    }

    void execute_to_result() {

        EmitterCollector  emittCollect(source, totalf, &totalDiff);
        Worker wrk(vd);

        vector<unique_ptr<ff_node> > W;
        for(int i=0;i<nw;++i) W.push_back(make_unique<Worker>(wrk));

        ff_Farm<Task> farm(move(W), emittCollect);
        farm.remove_collector(); // remove the collector (it is present by default in the ff_Farm)
        farm.wrap_around();

        if (farm.run_and_wait_end()<0) {
            error("running farm");
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
            utimer u("MasterWorker",&elapsed);
            
            EmitterCollector  emittCollect(source, totalf, &totalDiff);
            Worker wrk(vd);

            vector<unique_ptr<ff_node> > W;
            for(int i=0;i<nw;++i) W.push_back(make_unique<Worker>(wrk));

            ff_Farm<Task> farm(move(W), emittCollect);
            farm.remove_collector(); // remove the collector (it is present by default in the ff_Farm)
            farm.wrap_around();

            if (farm.run_and_wait_end()<0) {
                error("running farm");
                exit(1);
            }
        }

        cleanUp();
        exit(0);
    }
};