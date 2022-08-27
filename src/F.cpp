// class Task {
//     public:
//         //Mat frame;
//         ushort detected;

//         Task():detected(0) {}
// };

class Task {
    public:
        //Mat frame;
        ushort detected;

        Task():detected(0) {}
};

struct Emitter: ff_node_t<Task> {
    //VideoCapture* source;
    int totalf;

    //Emitter(VideoCapture* source, int totalf):source(source),totalf(totalf) {}
    Emitter(int totalf):totalf(totalf) {}

    Task* svc(Task *) {
        //Task* tsk = new Task();
        //Mat frame;
        for(int f=0; f<totalf-1; f++) {
            //ERROR(!source->read(frame),"Error in read frame operation")
            //tsk->frame = frame; // crea una copia??
            active_delay(100);
            Task* tsk = new Task();
            tsk->detected = f;

            //ERROR(!source->read(tsk->frame),"Error in read frame operation") //Error in a certain iteration

            cout << "Emitter send out " <<  tsk->detected << endl;
            ff_send_out(tsk);
        }
        return EOS;
    }

};

struct Worker: ff_node_t<Task> {
    //VideoDetection* vd;

    //Worker(VideoDetection* vd):vd(vd) {}

    Task* svc(Task * tsk) { 
        //Mat frame = tsk->frame;
        //tsk->detected = vd->composition(frame);
        //tsk->detected = vd->composition(tsk->frame);
        cout << "worker " << get_my_id() << " received " << tsk->detected << endl;
        tsk->detected *= 10;
        return tsk; 
    }
};

struct Collector: ff_node_t<Task> {
    int totalf;
    ulong totalDiff;

    Collector(int totalf):totalf(totalf),totalDiff(0) {}

    Task* svc(Task * tsk) { 
        //float &t = *task;
        //ushort &t = *(tsk->detected);
        //std::cout<< "thirdStage received " << t << "\n";
        //totalDiff += t;
        cout<< "Collector received " << tsk->detected << endl;
        totalDiff += tsk->detected;
        delete tsk;
        return GO_ON; 
    }
    void svc_end() { 
        cout << "Total frame: " << totalf << endl;
        cout << "Total diff: " << totalDiff << endl;
        }
};


class FastFlow {
    private:
    VideoCapture* source;  // Source of video
    int width,height;      // Shapes of frame
    int totalf;            // Number of total frame in the video
    VideoDetection* vd;
    Mat* background;       // Background images used for comparisons
    //ulong totalDiff = 0 ;  // Variable used to accomulate frame "detected"
    int nw;                // Farm-Worker(Farm)

    void cleanUp() {
        source->release();
        delete background;
        delete source;
    }
    public:
    FastFlow(const string path,const float k,const int nw):
        nw(nw) { 

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

        //Emitter  emitter(source, totalf);
        Emitter  emitter(totalf);
        //Worker wrk(vd);
        Worker wrk;
        Collector  collector(totalf);

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


        // cout << "Total frame: " << totalf << endl;
        // cout << "Total diff: " << totalDiff << endl;
        cleanUp();
        exit(0);
    }

    // void execute_to_stat() {

    //     ff_farm farm;  

    //     // both are defined in fastflow_a.cpp
    //     ff_loader loader(*source);
    //     ff_detect detect(&totalDiff);

    //     farm.add_collector(&detect);
    //     farm.add_emitter(&loader);


    //     vector<ff_node*> workers(f_nw);
    //     for(int i=0;i<f_nw;++i) {
    //         // build worker pipeline 
    //         ff_pipeline* pipe = new ff_pipeline;
    //         pipe->add_stage(new toGrayMap(source,dx,g_nw));
    //         pipe->add_stage(new toBlurMap(source,dx,c_nw,background,k));
    //         workers[i] = pipe;
    //     }
    //     farm.add_workers(move(workers));
    //     farm.set_scheduling_ondemand();

    //     long el;
    //     {
    //         utimer u("",&el);
    //         farm.run_and_wait_end();
    //     } 
    //     cout << el << endl;
    //     cleanUp();
    //     exit(0);
    // }
};