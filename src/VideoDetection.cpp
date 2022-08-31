class VideoDetection {
    protected:
    
        const int width,height; // Shape of frames
        const int ksize;        // Number of kernel's pixel (square kernel)
        const long fsize;       // Number of frame's pixels
        const int offset;       // (ksize-1)/2
        const float k;          // % of pixels that must be different to trigger "detection"
        Mat* background;        // Background image used to comparisons

    public:
        VideoDetection(const int width,const int height,const float k):
            width(width),height(height),ksize(9),
            fsize(width * height),offset(1),k(k),background(nullptr) { }

        void setBackground(Mat* background) {
            this->background = background;
        }

        void RGBtoGray(const Mat input, Mat* output) {
            int i,j;

            for (i = 0; i < height; i++) {
                for (j = 0; j < width; j++){
                    output->at<uchar>(i, j) = (input.at<Vec3b>(i, j)[2] + input.at<Vec3b>(i, j)[1] + input.at<Vec3b>(i, j)[0]) / 3;
                }
            }
        }

        void RGBtoGray_pad(const Mat input, Mat* output) {
            int i,j;

            for (i = 0; i < height; i++) {
                for (j = 0; j < width; j++){
                    output->at<uchar>(i+offset, j+offset) = (input.at<Vec3b>(i, j)[2] + input.at<Vec3b>(i, j)[1] + input.at<Vec3b>(i, j)[0]) / 3;
                }
            }
        }

        void smoothing(Mat* input, Mat* output) {       
            int i,j,r,c,col_LB,col_UB,row_LB,row_UB; 
            uchar sum;

            for (i = 0; i < height ; i++) {
                for (j = 0; j < width ; j++) {
                    sum = 0;
                    if(i>0 && i<height-1 && j>0 && j<width-1) {
                        for(r=-offset;r<=offset;r++)
                            for(c=-offset;c<=offset;c++) {
                                sum += input->at<uchar>(i+r,j+c);
                            }
                        output->at<uchar>(i, j) = sum/ksize;
                    }
                    else {
                        row_LB = i > 0 ? -offset : 0;
                        row_UB = i < height-1 ? offset : 0;
                        col_LB = j > 0 ? -offset : 0;
                        col_UB = j < width-1 ? offset : 0;
                        for(r=row_LB;r<=row_UB;r++)
                            for(c=col_LB;c<=col_UB;c++) {
                                sum += input->at<uchar>(i+r,j+c);
                            }
                        output->at<uchar>(i, j) = sum/ksize;
                    }
                }
            }
        }

        void smoothing_pad(Mat* input, Mat* output) {       
            int i,j,r,c;
            uchar sum;

            for (i = 0; i < height ; i++) {
                for (j = 0; j < width ; j++) {
                    sum = 0;
                    for(r=-offset;r<=offset;r++)
                        for(c=-offset;c<=offset;c++) 
                            sum += input->at<uchar>(i+offset+r, j+offset+c);
                    output->at<uchar>(i,j) = sum / ksize;
                }
            }
        }

        ushort detection(Mat* input) {
            int i,j;
            ulong diff = 0;
            float perc;

            for (i = 0; i < height; i++) {
                for (j = 0; j < width; j++){
                    diff += background->at<uchar>(i, j) != input->at<uchar>(i, j) ;
                }
            }

            // "Differents pixels" are divided by all pixels to obtain a percentage
            perc = (((float)diff)/fsize);  
            return  perc > k ;       
        }

        ushort composition(const Mat input) {    
            const ushort layers = 3;
            int i,j,c,r,col_LB,col_UB,row_LB,row_UB; 
            uchar sum;
            float perc;
            ulong diff = 0;

            for (i = 0; i < height ; i++) {
                for (j = 0; j < width ; j++) {
                    sum = 0;
                    if(i>0 && i<height-1 && j>0 && j<width-1) {
                        for(r=-offset;r<=offset;r++)
                            for(c=-offset;c<=offset;c++) {
                                sum += (input.at<Vec3b>(i+r, j+c)[2] + input.at<Vec3b>(i+r, j+c)[1] + input.at<Vec3b>(i+r, j+c)[0]) / layers;
                            }
                        diff += background->at<uchar>(i, j) != sum / ksize;
                    }
                    else {
                        row_LB = i > 0 ? -offset : 0;
                        row_UB = i < height-1 ? offset : 0;
                        col_LB = j > 0 ? -offset : 0;
                        col_UB = j < width-1 ? offset : 0;
                        for(r=row_LB;r<=row_UB;r++)
                            for(c=col_LB;c<=col_UB;c++) {
                                sum += (input.at<Vec3b>(i+r, j+c)[2] + input.at<Vec3b>(i+r, j+c)[1] + input.at<Vec3b>(i+r, j+c)[0]) / layers;
                            }
                        diff += background->at<uchar>(i, j) != sum / ksize;
                    }
                }
            }

            // "Differents pixels" are divided by all pixels to obtain a percentage
            perc = (((float)diff)/fsize);
            return  perc > k ;        
        }

        ushort composition_pad(const Mat input) {
            const ushort layers = 3;
            int i,j,r,c;
            //int i,j;
            uchar sum;
            float perc;
            ulong diff = 0;
            Mat padded = Mat(height+1,width+1,CV_8UC1,BLACK);

            for (i = 0; i < height ; i++)
                for (j = 0; j < width ; j++)
                    padded.at<uchar>(i+offset,j+offset) = (input.at<Vec3b>(i, j)[2] + input.at<Vec3b>(i, j)[1] + input.at<Vec3b>(i, j)[0]) / layers;

            for (i = 0; i < height ; i++) {
                for (j = 0; j < width ; j++) {
                    sum = 0;
                     for(r=-offset;r<=offset;r++)
                         for(c=-offset;c<=offset;c++) 
                             sum += padded.at<uchar>(i+offset+r, j+offset+c);
                    //sum = padded.at<uchar>(i+offset-1, j+offset-1)+padded.at<uchar>(i+offset-1, j+offset)+padded.at<uchar>(i+offset-1, j+offset+1)+padded.at<uchar>(i+offset, j+offset-1)+padded.at<uchar>(i+offset, j+offset)+padded.at<uchar>(i+offset, j+offset+1)+padded.at<uchar>(i+offset+1, j+offset-1)+padded.at<uchar>(i+offset+1, j+offset)+padded.at<uchar>(i+offset+1, j+offset+1);

                    diff += background->at<uchar>(i, j) != sum/ksize;
                }
            }

            // "Differents pixels" are divided by all pixels to obtain a percentage
            perc = (((float)diff)/fsize);   
            // if perc > k then the frame is "different" from background
            return  perc > k ;        
        }
        
};