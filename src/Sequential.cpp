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
            Mat *aux = new Mat(height,width,CV_8UC1,BLACK); // Auxiliar memory frame
            this->background = new Mat(height,width,CV_8UC1,BLACK);       
            ERROR(!source->read(frame),"Error in read frame operation") // Take the fist frame of the video
            vd->RGBtoGray(frame,aux);
            vd->smoothing(aux,this->background);
            vd->setBackground(this->background);
            delete aux;
    }

    void run() {
        // For each frame on the video (starting from 2nd frame)

        Mat frame;

        // Auxiliar memory frame
        // Mat *aux_gray = new Mat(height,width,CV_8UC1,BLACK);
        // Mat *aux_smooth = new Mat(height,width,CV_8UC1,BLACK);

        for(int f=0;f<totalf-1;f++) {
            ERROR(!source->read(frame),"Error in read frame operation")

            // vd->RGBtoGray(frame,aux_gray);
            // vd->smoothing(aux_gray,aux_smooth);
            // totalDiff += vd->detection(aux_smooth);

            totalDiff += vd->composition(frame);
        }

        // Clean memory on heap
        // delete aux_gray;
        // delete aux_smooth;

        cout << "Number of frames: " << totalf << endl;
        cout << "Number of frames with motion detected: " << totalDiff << endl;

        cleanUp();
        exit(0);
    }

    void time() {
        // For each frame on the video (stating from 2nd frame)

        Mat frame;

        // Auxiliar memory frame
        // Mat *aux_gray = new Mat(height,width,CV_8UC1,BLACK);
        // Mat *aux_smooth = new Mat(height,width,CV_8UC1,BLACK);

        long elapsed_time;
        {   
            utimer u("Sequential",&elapsed_time);

            for(int f=0;f<totalf-1;f++) {
                ERROR(!source->read(frame),"Error in read frame operation")

                // vd->RGBtoGray(frame,aux_gray);
                // vd->smoothing(aux_gray,aux_smooth);
                // totalDiff += vd->detection(aux_smooth);

                totalDiff += vd->composition(frame);
            }
        }

        // Clean memory on heap
        // delete aux_gray;
        // delete aux_smooth;

        cleanUp();
        exit(0);
    }

    void each_time() {
        // For each frame on the video (stating from 2nd frame)

        Mat frame;
        
        // Auxiliar memory frame
        Mat *aux_gray = new Mat(height,width,CV_8UC1,BLACK);
        Mat *aux_smooth = new Mat(height,width,CV_8UC1,BLACK);

        ulong tot_s1 = 0, tot_sc =0, tot_s2 = 0,tot_s3 = 0,tot_s4 = 0;

        for(int f=1;f<totalf;f++) {
            
            long elapsed_time;
            {   
                utimer u("READ",&elapsed_time);
                ERROR(!source->read(frame),"Error in read frame operation")
            }
            tot_s1 += elapsed_time;
            {
                utimer u("COMPOSITION",&elapsed_time);
                totalDiff += vd->composition(frame);
            }
            tot_sc += elapsed_time;
            {   
                utimer u("RGBtoGRAY",&elapsed_time);
                vd->RGBtoGray(frame,aux_gray);
            }
            tot_s2 += elapsed_time;
            {   
                utimer u("SMOOTHING",&elapsed_time);
                vd->smoothing(aux_gray,aux_smooth);
            }
            tot_s3 += elapsed_time;            
            {   
                utimer u("DETECTION",&elapsed_time);
                totalDiff += vd->detection(aux_smooth);
            }
            tot_s4 += elapsed_time;
        }
        cout << "READ: " << tot_s1/(totalf-1) << ", COMPOSITION: " << tot_sc/(totalf-1) << ", RGBtoGray: " << tot_s2/(totalf-1) << ", SMOOTHING: " << tot_s3/(totalf-1) << ", DETECTION: " << tot_s4/(totalf-1) << endl;
        
        // Clean memory on heap
        delete aux_gray;
        delete aux_smooth;

        cleanUp();
        exit(0);      
    }

};