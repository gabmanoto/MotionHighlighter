#include <opencv2/opencv.hpp>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <filesystem>
#include <fstream>

#include <chrono>
#include <ctime>

#include <bits/stdc++.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace cv;
using namespace std;

const char* params
    = "{ help h | | Print Usage}"
      "{ video v | input/demo.webm | Path to a video }";

const Scalar WHITE = Scalar(255, 255, 255), BLACK = Scalar(0, 0, 0), BLUE = Scalar(255, 0, 0), GREEN = Scalar(0, 255, 0), RED = Scalar(0, 0, 255), YELLOW = Scalar(0, 255, 255);

int minFramesToStartRecord = 30;
int maxFramesWithoutDetection = 60;
int maxFramesToRecord = 1800;
int minFramesToSnapshot = 60;
string currDirName, currSubDirName;

void createDir(char* dirName);
void exportInfoJSON();

float frameRate, videoTimeElapsed, videoTimeSecs, currentFrameCount, totalFrameCount, frameHeight, frameWidth, fourCC;
vector<int> motionStartTimesSecs, motionEndTimesSecs;

int main(int argc, char* argv[])
{
    auto start = chrono::system_clock::now();
    time_t s_time = chrono::system_clock::to_time_t(start);
    string start_time = ctime(&s_time);
    cout << "Started processing at " << s_time;
    
    currDirName = "output/" + start_time;
    char *cstr = new char[currDirName.length() + 1];
    strcpy(cstr, currDirName.c_str());
    createDir(cstr);

    CommandLineParser parser(argc, argv, params);
    
    parser.about("This program analyses videos to detect motion and outputs video clips highlighting the motion detections.");

    if (parser.has("help"))
    {
        parser.printMessage();
        return 0;
    }

    Mat currentFrame, nextFrame, currentFrame_gray, nextFrame_gray, currentFrame_blur, nextFrame_blur, morph, diff, thresh;

    String videoPath = parser.get<String>("video");
    cout << videoPath << endl;
    VideoCapture capture(videoPath);
    if(!capture.isOpened())
    {
        cerr << "Unable to open: " << parser.get<String>("video") << endl;
    }

    frameRate = capture.get(CAP_PROP_FPS);
    totalFrameCount = capture.get(CAP_PROP_FRAME_COUNT);
    videoTimeSecs = int(totalFrameCount/frameRate);
    frameHeight = capture.get(CAP_PROP_FRAME_HEIGHT);
    frameWidth = capture.get(CAP_PROP_FRAME_WIDTH); 
    fourCC = capture.get(CAP_PROP_FOURCC);

    cout << "Video frame rate: " << frameRate << endl; cout << endl;
    cout << "Video total frame count: " << totalFrameCount << endl; cout << endl;
    cout << "Video frame height: " << frameHeight << endl; cout << endl;
    cout << "Video frame width: " << frameWidth << endl; cout << endl;
    cout << "Video codec: " << fourCC << endl; cout << endl;
    cout << "-------------------------------------------------------------------------------" << endl; cout << endl;

    capture.read(currentFrame);
    capture.read(nextFrame);
    int key = 0;
    bool isRecording = false;
    int detectionCounter = 0;
    int noDetectionCounter = 0;
    int framesStored = 0;
    int momentCounter = 0;
    
    vector<Mat> detectionFrames;
    while(capture.isOpened() && key != 27)
    {
        currentFrameCount = capture.get(CAP_PROP_POS_FRAMES) - 1;
        
        videoTimeElapsed = int(capture.get(CAP_PROP_POS_MSEC)/1000);
        //cout << videoTimeElapsed << endl;

        if(currentFrameCount == totalFrameCount - 1)
        {
            exportInfoJSON();
            break;
        } 
     
        //cout << currentFrameCount << endl;
        cvtColor(currentFrame, currentFrame_gray, COLOR_BGR2GRAY);
        cvtColor(nextFrame, nextFrame_gray, COLOR_BGR2GRAY);

        GaussianBlur(currentFrame_gray, currentFrame_blur, Size(5, 5), 0);
        GaussianBlur(nextFrame_gray, nextFrame_blur, Size(5, 5), 0);

        absdiff(currentFrame_blur, nextFrame_blur, diff);

        threshold(diff, thresh, 30, 255.0, THRESH_BINARY);

        morph = thresh.clone();

        for(int i = 0; i < 3; i++)
        {
            dilate(morph, morph, getStructuringElement(MORPH_RECT, Size(7, 7)));
        }

        vector<vector<Point> > contours;
        findContours(morph, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        Mat img_out = nextFrame.clone();

        vector<vector<Point>> convexHulls(contours.size());
        for(int i = 0; i < contours.size(); i++)
        {
            convexHull(contours[i], convexHulls[i]);
        }

        bool isDetection = false;
        for (int i = 0; i < convexHulls.size(); i++)
        {
            Rect rect = boundingRect(convexHulls[i]);
            double aspectRatio, diagonalSize;
            aspectRatio = (double)rect.width / (double)rect.height;
            diagonalSize = sqrt(pow(rect.width, 2) + pow(rect.height, 2));

            if(aspectRatio > 0.3 && aspectRatio < 4.0 && rect.width > 50.0 && rect.height > 50.0 && diagonalSize > 50.0)
            {
                isDetection = true;
                rectangle(img_out, rect.tl(), rect.br(), RED, 1);
            }
        }

        if(isDetection)
        {
            detectionCounter++;
            noDetectionCounter = 0;
            detectionFrames.push_back(currentFrame);
            
        }
        else
        {
            noDetectionCounter++;
            if(noDetectionCounter >= 2 && detectionCounter > 0)
            {
                detectionCounter = 0;
                if(!isRecording)
                {
                    detectionFrames.clear();
                }
            }
            
        }
        if(detectionCounter >= minFramesToStartRecord && isRecording != true)
        {
            isRecording = true;
            motionStartTimesSecs.push_back(videoTimeElapsed);
            momentCounter++;
            currSubDirName = currDirName + "/moment-" + to_string(momentCounter);
            char *cstr = new char[currSubDirName.length() + 1];
            strcpy(cstr, currSubDirName.c_str());
            createDir(cstr);
            cout << "isRecording changed to true" << endl;
            cout << "Started recording" << endl;
        }

        if(isRecording == true)
        {
            detectionFrames.push_back(currentFrame);
            framesStored++;
            if(framesStored == int(maxFramesWithoutDetection/2))
            {
                imwrite(currSubDirName + "/img-" + to_string(currentFrameCount) + ".jpg", currentFrame);
            }
            if(framesStored >= maxFramesToRecord || noDetectionCounter >= maxFramesWithoutDetection || currentFrameCount == totalFrameCount - 1)
            {
               if(framesStored >= maxFramesToRecord)
                {
                    cout << "Max Frames Stored. " << framesStored << endl;
                }
                if(noDetectionCounter >= 300)
                {
                    cout << "Motion Detection Lost." << endl;
                }
                if(currentFrameCount == totalFrameCount - 1)
                {
                    cout << "Last frame reached." << endl;
                }  

                string videoFilename = currSubDirName + "/vid-" + to_string(currentFrameCount) + ".avi";
                string txtFileName = currSubDirName + "/vidInfo-" + to_string(currentFrameCount) + ".json";
                cout << videoFilename << endl;
                VideoWriter video(videoFilename, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), frameRate, Size(frameWidth, frameHeight));
                for(int i = 0; i < detectionFrames.size(); i++)
                {
                    video.write(detectionFrames.front());
                    detectionFrames.erase(detectionFrames.begin());
                }
                video.release();
                motionEndTimesSecs.push_back(videoTimeElapsed);
                framesStored = 0;
                isRecording = false;
                cout << "Video Stored, isRecording changed to false." << endl;
            }
        }

        imshow("Original", currentFrame);
        //imshow("Threshold", thresh);
        //imshow("Morphed", morph);
        //imshow("Output", img_out);

        currentFrame = nextFrame.clone();
        capture.read(nextFrame);

        key = waitKey(30);
        if(key == 27)
        {
            exportInfoJSON();
            break;
        }
    }
    return 0;
}

void createDir(char* dirName)
{
    if (mkdir(dirName, 0777) == -1)
    {
        cerr << "Error : " << strerror(errno) << endl;
    }
    else
    {
        cout << "Directory Created" << endl;
    }
}

void exportInfoJSON()
{
    string txtFileName =  currDirName + "/vidInfo.json";
    ofstream myfile;
    myfile.open(txtFileName);
    myfile << "{\n\"Video Frame Rate\": \"" + to_string(frameRate) + "\",\n";
    myfile << "\"Video Total Frame Count\": \"" + to_string(totalFrameCount) + "\",\n";
    myfile << "\"Video Total Time in seconds\": \"" + to_string(totalFrameCount) + "\",\n";
    myfile << "\"Video Frame Height\": \"" + to_string(frameHeight) + "\",\n";
    myfile << "\"Video Frame Width\": \"" + to_string(frameWidth) + "\"";

    if(motionStartTimesSecs.size() > 0)
    {
        myfile <<",\n";
        myfile << "\"Motion Start Times In Seconds\": \"[" + to_string(motionStartTimesSecs[0]);
        for(int i = 1; i < motionStartTimesSecs.size(); i++)
        {
            myfile << ", " + to_string(motionStartTimesSecs[i]);
        }
        myfile << "]\"";
    }
    if(motionEndTimesSecs.size() > 0)
    {
        myfile <<",\n";
        myfile << "\"Motion End Times In Seconds\": \"[" + to_string(motionEndTimesSecs[0]);
        for(int i = 1; i < motionEndTimesSecs.size(); i++)
        {
            myfile << ", " + to_string(motionEndTimesSecs[i]);
        }
        myfile << "]\"";
    }
    myfile << "\n}";
    myfile.close();
}