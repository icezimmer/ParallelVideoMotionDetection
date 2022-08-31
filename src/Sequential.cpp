class Sequential {

    private:
    VideoCapture* source; // Source of video
    int width,height;     // Shape of frame
    int totalf;           // Number of total frame in the video
    VideoDetection* vd;   // VideoDetection class contains methods used to process images
    Mat* background;      // Background image used for comparisons
    ulong totalDiff = 0 ; // Variable used to accomulate frame "detected"

    void cleanUp() {
        source->release();
        delete background;
        delete source;
        delete vd;
    }
    
    public:
    /**
    Sequential approach.
    */
    Sequential(const string path,const float k) {
        
        // checking argument
        ERROR(path == "","path error")
        ERROR(k<= 0 || k>1,"%'of pixel must be between 0 and 1")

        // VideoCapture class used to retrieve frames
        this->source = new VideoCapture(path); 

        // Check if the video is opened
        ERROR(!source->isOpened(),"Error opening video")

        // Some useful information
        this->width  = source->get(CAP_PROP_FRAME_WIDTH);
        this->height = source->get(CAP_PROP_FRAME_HEIGHT);
        this->totalf = source->get(CAP_PROP_FRAME_COUNT);

        // We need at least 2 frame: one is the background, the other is the frame to compare
        ERROR(totalf<3,"Too short video")

        // methods like RGBtoGray, smoothing ecc..
        this->vd = new VideoDetection(width,height,k);

        // We retrieve the background ----
            Mat frame;
            Mat *aux = new Mat(height+1,width+1,CV_8UC1,BLACK); // Auxiliar memory frame
            this->background = new Mat(height,width,CV_8UC1,BLACK);       
            ERROR(!source->read(frame),"Error in read frame operation") // Take the fist frame of the video
            vd->RGBtoGray_pad(frame,aux);
            vd->smoothing_pad(aux,this->background);
            vd->setBackground(this->background);
            delete aux;
    }

    void run() {
        // For each frame on the video (starting from 2nd frame)

        Mat frame;
        for(int f=0;f<totalf-1;f++) {
            ERROR(!source->read(frame),"Error in read frame operation")
            totalDiff += vd->composition_pad(frame);
        }

        cout << "Number of frames: " << totalf << endl;
        cout << "Number of frames with motion detected: " << totalDiff << endl;

        cleanUp();
        exit(0);
    }

    void time() {
        // For each frame on the video (stating from 2nd frame)

        long elapsed_time;
        {   
            utimer u("Sequential",&elapsed_time);

            Mat frame;
            for(int f=0;f<totalf-1;f++) {
                ERROR(!source->read(frame),"Error in read frame operation")
                totalDiff += vd->composition_pad(frame);
            }
        }

        cleanUp();
        exit(0);
    }

    void stats() {
        // For each frame on the video (stating from 2nd frame)

        Mat frame;
        
        // Auxiliar memory frame
        Mat *aux_gray = new Mat(height,width,CV_8UC1,BLACK);
        Mat *aux_gray_pad = new Mat(height+1,width+1,CV_8UC1,BLACK);
        Mat *aux_smooth = new Mat(height,width,CV_8UC1,BLACK);

        ulong tot_r = 0, tot_g = 0,tot_gp = 0,tot_s = 0,tot_sp = 0,tot_d = 0,tot_c =0,tot_cp =0;

        for(int f=1;f<totalf;f++) {
            
            long elapsed_time;
            {   
                utimer u("READ",&elapsed_time);
                ERROR(!source->read(frame),"Error in read frame operation")
            }
            tot_r += elapsed_time;
            {   
                utimer u("RGBtoGRAY",&elapsed_time);
                vd->RGBtoGray(frame,aux_gray);
            }
            tot_g += elapsed_time;
            {   
                utimer u("RGBtoGRAY_pad",&elapsed_time);
                vd->RGBtoGray_pad(frame,aux_gray_pad);
            }
            tot_gp += elapsed_time;
            {   
                utimer u("SMOOTHING",&elapsed_time);
                vd->smoothing(aux_gray,aux_smooth);
            }
            tot_s += elapsed_time;
            {   
                utimer u("SMOOTHING_pad",&elapsed_time);
                vd->smoothing_pad(aux_gray_pad,aux_smooth);
            }
            tot_sp += elapsed_time;             
            {   
                utimer u("DETECTION",&elapsed_time);
                totalDiff += vd->detection(aux_smooth);
            }
            tot_d += elapsed_time;
                        {
                utimer u("COMPOSITION",&elapsed_time);
                totalDiff += vd->composition(frame);
            }
            tot_c += elapsed_time;
            {
                utimer u("COMPOSITION_pad",&elapsed_time);
                totalDiff += vd->composition_pad(frame);
            }
            tot_cp += elapsed_time;
        }
        cout << "READ: " << tot_r/(totalf-1) << ", RGBtoGray: " << tot_g/(totalf-1) << ", RGBtoGray_pad: " << tot_gp/(totalf-1) <<  ", SMOOTHING: " << tot_s/(totalf-1) << ", SMOOTHING_pad: " << tot_sp/(totalf-1) << ", DETECTION: " << tot_d/(totalf-1) << ", COMPOSITION: " << tot_c/(totalf-1) << ", COMPOSITION_pad: " << tot_cp/(totalf-1) << endl;
        cout << "std: " << tot_r+tot_g+tot_s+tot_d << endl;
        cout << "std_pad: " << tot_r+tot_gp+tot_sp+tot_d << endl;
        cout << "comp: " << tot_c << endl;
        cout << "comp_pad: " << tot_cp << endl;
        
        // Clean memory on heap
        delete aux_gray;
        delete aux_gray_pad;
        delete aux_smooth;

        cleanUp();
        exit(0);      
    }

};