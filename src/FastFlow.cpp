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
};

struct Emitter_oh: ff_node_t<Task> {
    const int totalf;
    Emitter_oh(const int totalf):totalf(totalf) {}
    Task* svc(Task *) {
        for(int f=0; f<totalf-1; f++) {
            Task* tsk = new Task();;
            ff_send_out(tsk);
        }
        return EOS;
    }
};

struct Worker: ff_node_t<Task> {
    VideoDetection* vd;

    Worker(VideoDetection* vd):vd(vd) {}

    Task* svc(Task * tsk) { 
        tsk->detected = vd->composition_pad(tsk->frame);
        return tsk; 
    }
};

struct Worker_oh: ff_node_t<Task> {
    Task* svc(Task * tsk) { 
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
};

struct Collector_oh: ff_node_t<Task> {
    Task* svc(Task * tsk) { 
        delete tsk;
        return GO_ON; 
    }
};


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

struct EmitterCollector_oh: ff_node_t<Task> {
    const int totalf;
    int n_tasks;
    EmitterCollector_oh(const int totalf):totalf(totalf),n_tasks(0) {}
    Task* svc(Task *tsk) {
        if (tsk == nullptr) {
            for(int f=0; f<totalf-1; f++) {
                Task* tsk = new Task();
                ff_send_out(tsk);
            }
            return GO_ON;
        }        
        delete tsk;
        if (++n_tasks == totalf-1) return EOS;
        return GO_ON;         
    }
};

class FastFlow {
    protected:
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
};

class Pipe : protected FastFlow {
    public:
        Pipe(const string path,const float k,const int nw) : FastFlow(path,k,nw) {}

        void run() {
            Emitter  emitter(source, totalf);
            Worker wrk(vd);
            Collector  collector(&totalDiff);

            ff_Farm<Task> farm(
                [&]() {
                    vector<unique_ptr<ff_node> > W;
                    for(int i=0;i<nw;++i)
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

        void time() {
            long elapsed;
            {
                utimer u("FastFlow (Pipeline)",&elapsed);
                Emitter  emitter(source, totalf);
                Worker wrk(vd);
                Collector  collector(&totalDiff);

                ff_Farm<Task> farm(
                    [&]() {
                        vector<unique_ptr<ff_node> > W;
                        for(int i=0;i<nw;++i)
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

        void overhead() {
            long elapsed;
            {
                utimer u("OVERHEAD [FastFlow (Pipeline)]",&elapsed);
                Emitter_oh  emitter(totalf);
                Collector_oh  collector;

                ff_Farm<Task> farm(
                    [&]() {
                        vector<unique_ptr<ff_node> > W;
                        for(int i=0;i<nw;++i)
                            W.push_back(make_unique<Worker_oh>());
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

class Farm : protected FastFlow {
    public:
        Farm(const string path,const float k,const int nw) : FastFlow(path,k,nw) {}

        void run() {

            Emitter  emitter(source, totalf);
            Worker wrk(vd);
            Collector  collector(&totalDiff);

            vector<unique_ptr<ff_node> > W;
            for(int i=0;i<nw;++i) W.push_back(make_unique<Worker>(wrk));

            ff_Farm<Task> farm(move(W), emitter, collector);

            if (farm.run_and_wait_end()<0) {
                error("running farm");
                exit(1);
            }

            cout << "Total frame: " << totalf << endl;
            cout << "Total diff: " << totalDiff << endl;
            cleanUp();
            exit(0);
        }

        void time() {

            long elapsed;
            {
                utimer u("FastFlow (Farm)",&elapsed);
                Emitter  emitter(source, totalf);
                Worker wrk(vd);
                Collector  collector(&totalDiff);

                vector<unique_ptr<ff_node> > W;
                for(int i=0;i<nw;++i) W.push_back(make_unique<Worker>(wrk));

                ff_Farm<Task> farm(move(W), emitter, collector);

                if (farm.run_and_wait_end()<0) {
                    error("running farm");
                    exit(1);
                }
            }

            cleanUp();
            exit(0);
        }

        void overhead() {

            long elapsed;
            {
                utimer u("OVERHEAD [FastFlow (Farm)]",&elapsed);
                Emitter_oh  emitter(totalf);
                Collector_oh  collector;

                vector<unique_ptr<ff_node> > W;
                for(int i=0;i<nw;++i) W.push_back(make_unique<Worker_oh>());

                ff_Farm<Task> farm(move(W), emitter, collector);

                if (farm.run_and_wait_end()<0) {
                    error("running farm");
                    exit(1);
                }
            }

            cleanUp();
            exit(0);
        }
};

class MasterWorker : protected FastFlow {
    public:
        MasterWorker(const string path,const float k,const int nw) : FastFlow(path,k,nw) {}

        void run() {

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

        void time() {

            long elapsed;
            {
                utimer u("FastFlow (MasterWorker)",&elapsed);
                
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

        void overhead() {

            long elapsed;
            {
                utimer u("OVERHEAD [FastFlow (MasterWorker)]",&elapsed);
                
                EmitterCollector_oh  emittCollect(totalf);

                vector<unique_ptr<ff_node> > W;
                for(int i=0;i<nw;++i) W.push_back(make_unique<Worker_oh>());

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