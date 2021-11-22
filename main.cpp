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

#include "FaceDetector.h"

using namespace cv;
using namespace std;

const Scalar WHITE = Scalar(255, 255, 255), BLACK = Scalar(0, 0, 0), BLUE = Scalar(255, 0, 0), GREEN = Scalar(0, 255, 0), RED = Scalar(0, 0, 255), YELLOW = Scalar(0, 255, 255);

const char* params
    = "{ help h | | Print Usage}"
      "{ input i | input/demo.webm | input path to video }"
      "{ output o | output/ | path to output folder }"
      "{ record r | 0 | Record motion video clip: 0 for only snapshot images or 1 for record short video clips of motion detection.}"
      "{ debug d | 0 | Debug modes: 0 for no debug mode or 1 for debug mode}"
      "{ method m | 0 | Detection methods: 0 for background subtraction or 1 for face detection using neural network.}";

int minFramesToStartRecord = 30;
int maxFrameTolerationWithoutDetection = 60;
int maxFramesVideo = 1800;
int minFramesSnapshot = 60;
int frameForSnapshot = 5;

string outputDir, currDirName, currSubDirName;

void createDir(char* dirName);
void exportInfoJSON();

float frameRate, videoTimeElapsed, videoTimeSecs, currentFrameCount, totalFrameCount, frameHeight, frameWidth, fourCC;
vector<int> motionStartTimesSecs, motionEndTimesSecs;
Mat currentFrame, nextFrame, currentFrame_gray, nextFrame_gray, currentFrame_blur, nextFrame_blur, morph, diff, thresh, debugFrame;
int recordMode, debugMode, method;
time_t sys_startTime;

int main(int argc, char* argv[])
{
    auto sys_start = chrono::system_clock::now();
    sys_startTime = chrono::system_clock::to_time_t(sys_start);
    string str_sysStartTime = ctime(&sys_startTime);
        
    auto start = std::chrono::high_resolution_clock::now();
    
    CommandLineParser parser(argc, argv, params); 
    parser.about("This program analyses videos to detect motion and outputs video clips highlighting the motion detections.");

    String inputPath = parser.get<String>("input");
    String outputPath = parser.get<String>("output");

    if (parser.has("help"))
    {
        parser.printMessage();
        return 0;
    }
    
    string recordModeStr = parser.get<String>("record");
    if(recordModeStr == "0")
    {
        recordMode = 0;
    }
    else if(recordModeStr == "1")
    {
        recordMode = 1;
    }
    else
    {
        cerr << "Incorrect record mode specified" << endl;
        return -1;
    }

    string debugModeStr = parser.get<String>("debug");
    if(debugModeStr == "0")
    {
        debugMode = 0;
    }
    else if(debugModeStr == "1")
    {
        debugMode = 1;
    }
    else
    {
        cerr << "Incorrect debug mode specified" << endl;
        return -1;
    }
    
    string methodStr = parser.get<String>("method");
    if(methodStr == "0")
    {
        method = 0;
    }
    else if(methodStr == "1")
    {
        method = 1;
    }
    else
    {
        cerr << "Incorrect method specified" << endl;
        return -1;
    }
        
    currDirName = outputPath + str_sysStartTime;
    char *cstr = new char[currDirName.length() + 1];
    strcpy(cstr, currDirName.c_str());
    createDir(cstr);

    VideoCapture capture(inputPath);
    
    if(!capture.isOpened())
    {
        cerr << "Unable to open: " << parser.get<String>("input") << endl;
    }
    
    frameRate = capture.get(CAP_PROP_FPS);
    totalFrameCount = capture.get(CAP_PROP_FRAME_COUNT);
    videoTimeSecs = int(totalFrameCount/frameRate);
    frameHeight = capture.get(CAP_PROP_FRAME_HEIGHT);
    frameWidth = capture.get(CAP_PROP_FRAME_WIDTH); 
    fourCC = capture.get(CAP_PROP_FOURCC);

    if(debugMode == 1)
    {
        cout << "-----------------------------------------------------------------------------" << endl; cout << endl;
        cout << "Started processing at: " << str_sysStartTime << endl;
        cout << "Input Path: " << inputPath << endl;
        cout << "Output Path: " << outputPath << endl;
        cout << "Video frame rate: " << frameRate << endl; cout << endl;
        cout << "Video total frame count: " << totalFrameCount << endl; cout << endl;
        cout << "Video frame height: " << frameHeight << endl; cout << endl;
        cout << "Video frame width: " << frameWidth << endl; cout << endl;
        cout << "Video codec: " << fourCC << endl; cout << endl;
        cout << "-------------------------------------------------------------------------------" << endl; cout << endl;
    }

    if(method==1){ FaceDetector face_detector; }
    
    capture.read(currentFrame);
    capture.read(nextFrame);

    if(debugMode == 1){ debugFrame = currentFrame.clone(); }

    int key = 0;
    bool isRecording = false;
    int detectionCounter = 0;
    int noDetectionCounter = 0;
    int framesRecorded = 0;
    int momentCounter = 0;
    
    vector<Mat> detectionFrames;
    while(capture.isOpened() && key != 27)
    {
        currentFrameCount = capture.get(CAP_PROP_POS_FRAMES) - 1;        
        videoTimeElapsed = int(capture.get(CAP_PROP_POS_MSEC)/1000);

        if(currentFrameCount == totalFrameCount - 1)
        {
            exportInfoJSON();
            break;
        } 
        
        bool isDetection = false;
        
        if(method == 1)
        {
            auto rectangles = face_detector.detect_face_rectangles(currentFrame);
            if(rectangles.size() > 0)
            {
                isDetection = true;
                if(debugMode == 1)
                {
                    for(const auto & r : rectangles)
                    {
                        cv::rectangle(debugFrame, r, BLUE, 4);
                    }
                }
            }
        }
        else if(method == 0)
        {
            
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

            for (int i = 0; i < convexHulls.size(); i++)
            {
                Rect rect = boundingRect(convexHulls[i]);
                double aspectRatio, diagonalSize;
                aspectRatio = (double)rect.width / (double)rect.height;
                diagonalSize = sqrt(pow(rect.width, 2) + pow(rect.height, 2));

                if(aspectRatio > 0.3 && aspectRatio < 4.0 && rect.width > 50.0 && rect.height > 50.0 && diagonalSize > 50.0)
                {
                    isDetection = true;
                    if(debugMode == 1)
                    {
                        rectangle(debugFrame, rect.tl(), rect.br(), RED, 1);
                    }
                }
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
            currSubDirName = currDirName + "/" + to_string(momentCounter);
            char *cstr = new char[currSubDirName.length() + 1];
            strcpy(cstr, currSubDirName.c_str());
            createDir(cstr);
            if(debugMode == 1)
            {
                cout << "isRecording changed to true" << endl;
                cout << "Motion recording started..." << endl;
            }
            
        }
        if(isRecording == true)
        {
            framesRecorded++;
            if(framesRecorded == frameForSnapshot)
            {
                imwrite(currSubDirName + "/img-" + to_string(currentFrameCount) + ".jpg", currentFrame);
            }
            if(recordMode == 1)
            {
                detectionFrames.push_back(currentFrame);
                if(framesRecorded >= maxFramesVideo)
                {
                    string videoFilename = currSubDirName + "/vid-" + to_string(currentFrameCount) + ".avi";
                    cout << videoFilename << endl;
                    VideoWriter video(videoFilename, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), frameRate, Size(frameWidth, frameHeight));
                    for(int i = 0; i < detectionFrames.size(); i++)
                    {
                        video.write(detectionFrames.front());
                        detectionFrames.erase(detectionFrames.begin());
                    }
                    video.release();
                }
            }
            if(noDetectionCounter >= maxFrameTolerationWithoutDetection || currentFrameCount == totalFrameCount - 1)
            {
                if(recordMode == 1)
                {
                    string videoFilename = currSubDirName + "/vid-" + to_string(currentFrameCount) + ".avi";
                    VideoWriter video(videoFilename, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), frameRate, Size(frameWidth, frameHeight));
                    for(int i = 0; i < detectionFrames.size(); i++)
                    {
                        video.write(detectionFrames.front());
                        detectionFrames.erase(detectionFrames.begin());
                    }
                    video.release();
                }
            
                motionEndTimesSecs.push_back(videoTimeElapsed);
                framesRecorded = 0;
                isRecording = false;
                if(debugMode == 1) {cout << "isRecording changed to false" << endl;}
            }
        }
        if(debugMode == 1)
        {
            imshow("Debug Frame", debugFrame);
        }
        currentFrame = nextFrame.clone();
        capture.read(nextFrame);

        key = waitKey(30);
        if(key == 27)
        {
            exportInfoJSON();
            break;
        }
    }
    
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    if (debugMode == 1)
    {
        std::cout << "Elapsed time: " << elapsed.count() << " s\n";
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