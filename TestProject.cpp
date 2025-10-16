



#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>  // for rand()
#include <ctime>    // for time()
#include <cmath>

using namespace cv;
using namespace std;

// ===============================
// Global Structures & Variables
// ===============================
struct AIData {
    uint8_t ClassID;
    Point Centre;
    Rect Box;
};

struct TgtData {
    uint8_t ClassID;
    Point Centre;
    Rect Box;
    bool ValidData;
};

vector<AIData> TgtDataArr;
TgtData Target;

// ===============================
// Processing Function Declaration
// ===============================
void ProcessAI(Mat &frame, bool Flag, uint16_t X, uint16_t Y, uint16_t width, uint16_t height);

// ===============================
// Main Function
// ===============================
int main() {
    srand((unsigned int)time(nullptr)); // Seed RNG

    const int IMG_WIDTH = 640;
    const int IMG_HEIGHT = 480;
    Mat frame = Mat::zeros(Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC3);

    bool Flag = true; // true = consider all 0–3; false = only 0 & 1

    for (int iter = 0; iter < 20; ++iter) {  // 20 frames
        frame = Mat::zeros(Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC3); // clear frame

        // --- Generate random AI array ---
        TgtDataArr.clear();
        int numDetections = 10 + rand() % 11; // 10-20 detections per frame
        for (int i = 0; i < numDetections; ++i) {
            int classId = 1 + rand() % 5; // 1-5
            int x = rand() % IMG_WIDTH;
            int y = rand() % IMG_HEIGHT;
            int w = 20 + rand() % 50; // width 20-70
            int h = 20 + rand() % 50; // height 20-70
            TgtDataArr.push_back({(uint8_t)classId, Point(x, y), Rect(x - w/2, y - h/2, w, h)});
        }

        // --- Generate random AOI parameters ---
        uint16_t X = 50 + rand() % (IMG_WIDTH - 100);
        uint16_t Y = 50 + rand() % (IMG_HEIGHT - 100);
        uint16_t width  = 50 + rand() % 100;
        uint16_t height = 50 + rand() % 100;

        // --- Process the frame ---
        ProcessAI(frame, Flag, X, Y, width, height);

        // --- Display frame ---
        imshow("Dynamic AI Demo", frame);
        if (waitKey(500) == 27) break; // wait 200ms, break on ESC
    }

    return 0;
}

// ===============================
// Processing Function Definition
// ===============================
void ProcessAI(Mat &frame, bool Flag, uint16_t X, uint16_t Y, uint16_t width, uint16_t height)
{
    int IMG_WIDTH = frame.cols;
    int IMG_HEIGHT = frame.rows;
    Target = {0, Point(0,0), Rect(), false};

    Scalar white(255,255,255);
    int thin = 1;
    Point imgCenter(IMG_WIDTH/2, IMG_HEIGHT/2);

    // --- Draw quadrants ---
    line(frame, Point(imgCenter.x,0), Point(imgCenter.x,IMG_HEIGHT), white, thin);
    line(frame, Point(0,imgCenter.y), Point(IMG_WIDTH,imgCenter.y), white, thin);

    // --- Draw AI detections for ClassID 1–5 ---
    for (auto &data : TgtDataArr) {
        if (data.ClassID >= 1 && data.ClassID <= 5) {
            rectangle(frame, data.Box, white, thin);
            putText(frame, to_string(data.ClassID), data.Centre + Point(-8,-8), FONT_HERSHEY_SIMPLEX, 0.5, white, thin);
            circle(frame, data.Centre, 2, white, FILLED);
        }
    }

    // --- AOI Search ---
    Rect AOI(X - width/2, Y - height/2, width, height);
    bool found = false;
    int expandStep = 0;

    while(!found) {
        vector<AIData> candidates;
        AOI &= Rect(0,0,IMG_WIDTH,IMG_HEIGHT);

        for (auto &data : TgtDataArr) {
            if (!Flag && !(data.ClassID == 1 || data.ClassID == 0)) continue; // only if flag=false

            if (AOI.contains(data.Centre)) candidates.push_back(data);
        }

        if (!candidates.empty()) {
            found = true;
            double minDist = 1e9;
            AIData best;
            for (auto &c : candidates) {
                double dist = norm(c.Centre - imgCenter);
                if (dist < minDist) {
                    minDist = dist;
                    best = c;
                }
            }
            Target = {best.ClassID, best.Centre, best.Box, true};
            break;
        }

        // Asymmetric exqpansion
        expandStep += 5;
        double xBias = (double)X / IMG_WIDTH;
        double yBias = (double)Y / IMG_HEIGHT;

        int leftExpand   = (int)(expandStep * (1.0 - xBias));
        int rightExpand  = (int)(expandStep * (xBias + 0.2));
        int topExpand    = (int)(expandStep * (1.0 - yBias));
        int bottomExpand = (int)(expandStep * (yBias + 0.2));

        AOI.x = X - (width/2 + leftExpand);
        AOI.y = Y - (height/2 + topExpand);
        AOI.width  = width + leftExpand + rightExpand;
        AOI.height = height + topExpand + bottomExpand;

        AOI &= Rect(0,0,IMG_WIDTH,IMG_HEIGHT);

        if (AOI.width >= IMG_WIDTH && AOI.height >= IMG_HEIGHT) {
            Target.ValidData = false;
            break;
        }
    }

    rectangle(frame, AOI, white, thin);

    if (Target.ValidData) {
        rectangle(frame, Target.Box, white, thin);
        circle(frame, Target.Centre, 3, white, FILLED);
        line(frame, imgCenter, Target.Centre, white, thin);
        putText(frame, "Target Class:" + to_string(Target.ClassID), Point(10,30), FONT_HERSHEY_SIMPLEX, 0.5, white, thin);
    } else {
        putText(frame, "No Target Found", Point(10,30), FONT_HERSHEY_SIMPLEX, 0.5, white, thin);
    }
}

























#if 0


#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>

using namespace cv;
using namespace std;

// ===============================
// Global Structures & Variables
// ===============================
struct AIData {
    uint8_t ClassID;
    Point Centre;
    Rect Box;
};

struct TgtData {
    uint8_t ClassID;
    Point Centre;
    Rect Box;
    bool ValidData;
};

// Global arrays
vector<AIData> TgtDataArr;
TgtData Target;

// ===============================
// Processing Function Declaration
// ===============================
void ProcessAI(Mat &frame, bool Flag, uint16_t X, uint16_t Y, uint16_t width, uint16_t height);

// ===============================
// Main Function
// ===============================
int main() {
    // --- Simulated detections (20 entries) ---
    TgtDataArr = {
        {0, {100, 150}, Rect(80, 130, 40, 40)},
        {1, {320, 230}, Rect(300, 210, 40, 40)},
        {2, {500, 300}, Rect(480, 280, 40, 40)},
        {3, {600, 400}, Rect(580, 380, 40, 40)},
        {0, {250, 200}, Rect(230, 180, 40, 40)},
        {1, {400, 260}, Rect(380, 240, 40, 40)},
        {2, {350, 100}, Rect(330, 80, 40, 40)},
        {3, {200, 400}, Rect(180, 380, 40, 40)},
        {4, {500, 100}, Rect(480, 80, 40, 40)},
        {5, {50,  50},  Rect(30, 30, 40, 40)},
        {0, {100, 400}, Rect(80, 380, 40, 40)},
        {1, {550, 150}, Rect(530, 130, 40, 40)},
        {2, {450, 350}, Rect(430, 330, 40, 40)},
        {3, {300, 400}, Rect(280, 380, 40, 40)},
        {0, {320, 100}, Rect(300, 80, 40, 40)},
        {1, {200, 240}, Rect(180, 220, 40, 40)},
        {2, {420, 200}, Rect(400, 180, 40, 40)},
        {3, {250, 350}, Rect(230, 330, 40, 40)},
        {6, {600, 100}, Rect(580, 80, 40, 40)},
        {7, {640, 470}, Rect(620, 450, 40, 40)}
    };

    // --- Example settings ---
    bool Flag = true;          // true = consider 0–3; false = consider only 0 & 1
    uint16_t X = 250, Y = 200; // AOI center
    uint16_t width = 100, height = 100;

    // --- Create dynamic image (any size) ---
    Mat frame = Mat::zeros(Size(640, 480), CV_8UC3);

    // --- Process detections ---
    ProcessAI(frame, Flag, X, Y, width, height);

    // --- Display result ---
    imshow("AI Target Processing", frame);
    waitKey(0);

    return 0;
}

// ===============================
// Processing Function Definition
// ===============================
void ProcessAI(Mat &frame, bool Flag, uint16_t X, uint16_t Y, uint16_t width, uint16_t height)
{
    // Dynamic image size
    int IMG_WIDTH = frame.cols;
    int IMG_HEIGHT = frame.rows;

    // Reset target
    Target = {0, Point(0, 0), Rect(), false};

    // Colors & drawing thickness
    Scalar white(255, 255, 255);
    int thin = 1;

    // Draw quadrants (cross lines)
    Point imgCenter(IMG_WIDTH / 2, IMG_HEIGHT / 2);
    line(frame, Point(imgCenter.x, 0), Point(imgCenter.x, IMG_HEIGHT), white, thin);
    line(frame, Point(0, imgCenter.y), Point(IMG_WIDTH, imgCenter.y), white, thin);

    // Draw detections for Class 0–3
    for (auto &data : TgtDataArr) {
        if (data.ClassID <= 3) {
            rectangle(frame, data.Box, white, thin);
            putText(frame, to_string(data.ClassID), data.Centre + Point(-8, -8),
                    FONT_HERSHEY_SIMPLEX, 0.5, white, thin);
            circle(frame, data.Centre, 2, white, FILLED);
        }
    }

    // Initialize AOI rectangle
    Rect AOI(X - width / 2, Y - height / 2, width, height);
    bool found = false;
    int expandStep = 0;

    // --- Main search loop ---
    while (!found) {
        vector<AIData> candidates;

        AOI &= Rect(0, 0, IMG_WIDTH, IMG_HEIGHT);

        // Collect valid detections within AOI
        for (auto &data : TgtDataArr) {
            if (data.ClassID > 3) continue;
            if (!Flag && !(data.ClassID == 0 || data.ClassID == 1))
                continue;
            if (AOI.contains(data.Centre))
                candidates.push_back(data);
        }

        if (!candidates.empty()) {
            found = true;

            // Find closest to image center
            double minDist = 1e9;
            AIData best;
            for (auto &c : candidates) {
                double dist = norm(c.Centre - imgCenter);
                if (dist < minDist) {
                    minDist = dist;
                    best = c;
                }
            }

            Target = {best.ClassID, best.Centre, best.Box, true};
            break;
        }

        // Smarter asymmetric ROI expansion
        expandStep += 5;
        double xBias = (double)X / IMG_WIDTH;
        double yBias = (double)Y / IMG_HEIGHT;

        int leftExpand   = (int)(expandStep * (1.0 - xBias));
        int rightExpand  = (int)(expandStep * (xBias + 0.2));
        int topExpand    = (int)(expandStep * (1.0 - yBias));
        int bottomExpand = (int)(expandStep * (yBias + 0.2));

        AOI.x = X - (width / 2 + leftExpand);
        AOI.y = Y - (height / 2 + topExpand);
        AOI.width  = width + leftExpand + rightExpand;
        AOI.height = height + topExpand + bottomExpand;

        AOI &= Rect(0, 0, IMG_WIDTH, IMG_HEIGHT);

        if (AOI.width >= IMG_WIDTH && AOI.height >= IMG_HEIGHT) {
            Target.ValidData = false;
            break;
        }
    }

    // Draw AOI rectangle
    rectangle(frame, AOI, white, thin);

    // Draw target + line
    if (Target.ValidData) {
        rectangle(frame, Target.Box, white, thin);
        circle(frame, Target.Centre, 3, white, FILLED);
        line(frame, imgCenter, Target.Centre, white, thin);
        putText(frame, "Target Class: " + to_string(Target.ClassID),
                Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.5, white, thin);
    } else {
        putText(frame, "No Target Found", Point(10, 30),
                FONT_HERSHEY_SIMPLEX, 0.5, white, thin);
    }
}

#endif



























#if 0


#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>

using namespace cv;
using namespace std;

struct AIData {
    uint8_t ClassID;
    Point Centre;
    Rect Box;
};

struct TgtData {
    uint8_t ClassID;
    Point Centre;
    Rect Box;
    bool ValidData;
};

int main() {
    const int IMG_WIDTH = 640;
    const int IMG_HEIGHT = 480;

    // --- Simulated AI detections (20 entries) ---
    vector<AIData> TgtDataArr = {
    		//        {0, {100, 150}, Rect(80, 130, 40, 40)},
    		//        {1, {320, 230}, Recqt(300, 210, 40, 40)},
//    		        {2, {500, 300}, Rect(480, 280, 40, 40)},
//    		        {3, {600, 400}, Rect(580, 380, 40, 40)},
    		//        {0, {250, 200}, Rect(230, 180, 40, 40)},
    		//        {1, {400, 260}, Rect(380, 240, 40, 40)},
    		//        {2, {350, 100}, Rect(330, 80, 40, 40)},
    		//        {3, {200, 400}, Rect(180, 380, 40, 40)},
    		//        {4, {500, 100}, Rect(480, 80, 40, 40)},   // ignored
    		        {5, {50,  50},  Rect(30, 30, 40, 40)},    // ignored
//    		        {0, {100, 400}, Rect(80, 380, 40, 40)},
//    		        {1, {550, 150}, Rect(530, 130, 40, 40)},
    		//        {2, {450, 350}, Rect(430, 330, 40, 40)},
    		//        {3, {300, 400}, Rect(280, 380, 40, 40)},
    		//        {0, {320, 100}, Rect(300, 80, 40, 40)},
    		//        {1, {200, 240}, Rect(180, 220, 40, 40)},
    		//        {2, {420, 200}, Rect(400, 180, 40, 40)},
    		//        {3, {250, 350}, Rect(230, 330, 40, 40)},
    		//        {6, {600, 100}, Rect(580, 80, 40, 40)},   // ignored
    		//        {7, {640, 470}, Rect(620, 450, 40, 40)}   // ignored
    };

    // --- Parameters ---
    bool Flag = true; // If true → sort among all 0–3, if false → only 0 & 1
    uint16_t X = 50, Y = 50;  // deliberately off-center ROI
    uint16_t width = 100, height = 100;

    // --- Output structure ---
    TgtData target = {0, Point(0, 0), Rect(), false};

    // --- Create black image ---
    Mat image = Mat::zeros(Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC3);
    Scalar white(255, 255, 255);
    int thin = 1;

    // --- Draw quadrant lines (thin) ---
    Point imgCenter(IMG_WIDTH / 2, IMG_HEIGHT / 2);
    line(image, Point(imgCenter.x, 0), Point(imgCenter.x, IMG_HEIGHT), white, thin);
    line(image, Point(0, imgCenter.y), Point(IMG_WIDTH, imgCenter.y), white, thin);

    // --- Draw bounding boxes for ClassID 0–3 ---
    for (auto &data : TgtDataArr) {
        if (data.ClassID <= 3) {
            rectangle(image, data.Box, white, thin);
            putText(image, to_string(data.ClassID), data.Centre + Point(-8, -8),
                    FONT_HERSHEY_SIMPLEX, 0.5, white, thin);
            circle(image, data.Centre, 2, white, FILLED);
        }
    }

    // --- AOI Search ---
    bool found = false;
    int expandStep = 0;
    Rect AOI(X - width / 2, Y - height / 2, width, height);

    while (!found) {
        vector<AIData> candidates;

        // Clip AOI inside image
        AOI &= Rect(0, 0, IMG_WIDTH, IMG_HEIGHT);

        // Collect valid detections within AOI
        for (auto &data : TgtDataArr) {
            if (data.ClassID > 3) continue;
            if (!Flag && !(data.ClassID == 0 || data.ClassID == 1))
                continue;

            if (AOI.contains(data.Centre))
                candidates.push_back(data);
        }

        if (!candidates.empty()) {
            found = true;

            // Pick closest to image center
            double minDist = 1e9;
            AIData best;
            for (auto &c : candidates) {
                double dist = norm(c.Centre - imgCenter);
                if (dist < minDist) {
                    minDist = dist;
                    best = c;
                }
            }

            target = {best.ClassID, best.Centre, best.Box, true};
            break;
        }

        // --- Smarter ROI expansion ---
        expandStep += 5;

        // Determine relative position of ROI center vs image center
        double xBias = (double)X / IMG_WIDTH;   // 0.0 = left, 1.0 = right
        double yBias = (double)Y / IMG_HEIGHT;  // 0.0 = top, 1.0 = bottom

        int leftExpand = (int)(expandStep * (1.0 - xBias));  // more if closer to right
        int rightExpand = (int)(expandStep * (xBias + 0.2)); // more if closer to left
        int topExpand = (int)(expandStep * (1.0 - yBias));
        int bottomExpand = (int)(expandStep * (yBias + 0.2));

        AOI.x = X - (width / 2 + leftExpand);
        AOI.y = Y - (height / 2 + topExpand);
        AOI.width = width + leftExpand + rightExpand;
        AOI.height = height + topExpand + bottomExpand;

        // Clip AOI to stay within image boundaries
        AOI &= Rect(0, 0, IMG_WIDTH, IMG_HEIGHT);

        if (AOI.width >= IMG_WIDTH && AOI.height >= IMG_HEIGHT) {
            target.ValidData = false;
            break;
        }
    }

    // --- Draw final AOI ---
    rectangle(image, AOI, white, thin);

    // --- Draw target and connecting line ---
    if (target.ValidData) {
        rectangle(image, target.Box, white, thin);
        circle(image, target.Centre, 3, white, FILLED);
        line(image, imgCenter, target.Centre, white, thin);
        putText(image, "Target Class: " + to_string(target.ClassID),
                Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.5, white, thin);
    } else {
        putText(image, "No Target Found", Point(10, 30),
                FONT_HERSHEY_SIMPLEX, 0.5, white, thin);
    }

    // --- Display Result ---
    imshow("AI Target Detection (Smart ROI)", image);
    waitKey(0);
    return 0;
}


#endif




























#if 0


// Working logic

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>

using namespace cv;
using namespace std;

struct AIData {
    uint8_t ClassID;
    Point Centre;
    Rect Box;
};

struct TgtData {
    uint8_t ClassID;
    Point Centre;
    Rect Box;
    bool ValidData;
};

int main() {
    const int IMG_WIDTH = 640;
    const int IMG_HEIGHT = 480;

    // --- Simulated AI detections (20 entries) ---
    vector<AIData> TgtDataArr = {
//        {0, {100, 150}, Rect(80, 130, 40, 40)},
//        {1, {320, 230}, Recqt(300, 210, 40, 40)},
        {2, {500, 300}, Rect(480, 280, 40, 40)},
        {3, {600, 400}, Rect(580, 380, 40, 40)},
//        {0, {250, 200}, Rect(230, 180, 40, 40)},
//        {1, {400, 260}, Rect(380, 240, 40, 40)},
//        {2, {350, 100}, Rect(330, 80, 40, 40)},
//        {3, {200, 400}, Rect(180, 380, 40, 40)},
//        {4, {500, 100}, Rect(480, 80, 40, 40)},   // ignored
        {5, {50,  50},  Rect(30, 30, 40, 40)},    // ignored
        {0, {100, 400}, Rect(80, 380, 40, 40)},
        {1, {550, 150}, Rect(530, 130, 40, 40)},
//        {2, {450, 350}, Rect(430, 330, 40, 40)},
//        {3, {300, 400}, Rect(280, 380, 40, 40)},
//        {0, {320, 100}, Rect(300, 80, 40, 40)},
//        {1, {200, 240}, Rect(180, 220, 40, 40)},
//        {2, {420, 200}, Rect(400, 180, 40, 40)},
//        {3, {250, 350}, Rect(230, 330, 40, 40)},
//        {6, {600, 100}, Rect(580, 80, 40, 40)},   // ignored
//        {7, {640, 470}, Rect(620, 450, 40, 40)}   // ignored
    };

    // --- Parameters ---
    bool Flag = true; // If true → sort among all 0–3, if false → only 0 & 1
    uint16_t X = 50, Y = 50, width = 50, height = 50;

    // --- Output structure ---
    TgtData target = {0, Point(0, 0), Rect(), false};

    // --- Create black image ---
    Mat image = Mat::zeros(Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC3);

    // --- White color and thin line width ---
    Scalar white(255, 255, 255);
    int thin = 1;

    // --- Draw quadrant lines (thin) ---
    Point imgCenter(IMG_WIDTH / 2, IMG_HEIGHT / 2);
    line(image, Point(imgCenter.x, 0), Point(imgCenter.x, IMG_HEIGHT), white, thin);
    line(image, Point(0, imgCenter.y), Point(IMG_WIDTH, imgCenter.y), white, thin);

    // --- Draw bounding boxes for ClassID 0–3 (thin white) ---
    for (auto &data : TgtDataArr) {
        if (data.ClassID <= 3) {
            rectangle(image, data.Box, white, thin);
            putText(image, to_string(data.ClassID), data.Centre + Point(-8, -8),
                    FONT_HERSHEY_SIMPLEX, 0.5, white, thin);
            circle(image, data.Centre, 2, white, FILLED);
        }
    }

    // --- AOI Search ---
    bool found = false;
    int expandStep = 0;
    Rect AOI(X - width / 2, Y - height / 2, width, height);

    while (!found) {
        vector<AIData> candidates;

        // Clip AOI inside image bounds
        AOI &= Rect(0, 0, IMG_WIDTH, IMG_HEIGHT);

        // Collect valid detections within AOI
        for (auto &data : TgtDataArr) {
            if (data.ClassID > 3) continue; // only 0–3

            // If Flag == false → only consider 0 & 1
            if (!Flag && !(data.ClassID == 0 || data.ClassID == 1))
                continue;

            if (AOI.contains(data.Centre))
                candidates.push_back(data);
        }

        if (!candidates.empty()) {
            found = true;

            // Select closest to image center
            double minDist = 1e9;
            AIData best;

            for (auto &c : candidates) {
                double dist = norm(c.Centre - imgCenter);
                if (dist < minDist) {
                    minDist = dist;
                    best = c;
                }
            }

            target.ClassID = best.ClassID;
            target.Centre = best.Centre;
            target.Box = best.Box;
            target.ValidData = true;
            break;
        }

        // Expand AOI by 5px on all sides
        expandStep += 5;
        AOI.x = X - (width / 2 + expandStep);
        AOI.y = Y - (height / 2 + expandStep);
        AOI.width = width + expandStep * 2;
        AOI.height = height + expandStep * 2;

        // Stop if AOI covers entire image
        if (AOI.width >= IMG_WIDTH && AOI.height >= IMG_HEIGHT) {
            target.ValidData = false;
            break;
        }
    }

    // --- Draw AOI (thin white box) ---
    rectangle(image, AOI, white, thin);

    // --- Draw target and connecting line (thin white) ---
    if (target.ValidData) {
        rectangle(image, target.Box, white, thin);
        circle(image, target.Centre, 3, white, FILLED);
        line(image, imgCenter, target.Centre, white, thin);
        putText(image, "Target Class: " + to_string(target.ClassID),
                Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.5, white, thin);
    } else {
        putText(image, "No Target Found", Point(10, 30),
                FONT_HERSHEY_SIMPLEX, 0.5, white, thin);
    }

    // --- Display Result ---
    imshow("AI Target Detection (Thin White)", image);
    waitKey(0);
    return 0;
}



#endif


































#if 0

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cmath>

using namespace cv;
using namespace std;

struct AIData {
    uint8_t ClassID;
    Point Centre;
    Rect Box;
};

struct TgtData {
    uint8_t ClassID;
    Point Centre;
    Rect Box;
    bool ValidData;
};

int main() {
    const int IMG_WIDTH = 640;
    const int IMG_HEIGHT = 480;

    // --- Simulated AI detections (20 entries) ---
    vector<AIData> TgtDataArr = {
        {0, {100, 150}, Rect(80, 130, 40, 40)},
        {1, {320, 230}, Rect(300, 210, 40, 40)},
        {2, {500, 300}, Rect(480, 280, 40, 40)},
        {3, {600, 400}, Rect(580, 380, 40, 40)},
        {0, {250, 200}, Rect(230, 180, 40, 40)},
        {1, {400, 260}, Rect(380, 240, 40, 40)},
        {2, {350, 100}, Rect(330, 80, 40, 40)},
        {3, {200, 400}, Rect(180, 380, 40, 40)},
        {4, {500, 100}, Rect(480, 80, 40, 40)},   // ignored
        {5, {50,  50},  Rect(30, 30, 40, 40)},    // ignored
        {0, {100, 400}, Rect(80, 380, 40, 40)},
        {1, {550, 150}, Rect(530, 130, 40, 40)},
        {2, {450, 350}, Rect(430, 330, 40, 40)},
        {3, {300, 400}, Rect(280, 380, 40, 40)},
        {0, {320, 100}, Rect(300, 80, 40, 40)},
        {1, {200, 240}, Rect(180, 220, 40, 40)},
        {2, {420, 200}, Rect(400, 180, 40, 40)},
        {3, {250, 350}, Rect(230, 330, 40, 40)},
        {6, {600, 100}, Rect(580, 80, 40, 40)},   // ignored
        {7, {640, 470}, Rect(620, 450, 40, 40)}   // ignored
    };

    // --- Parameters ---
    bool Flag = true; // If true → sort among all 0–3, if false → only 0 & 1
    uint16_t X = 320, Y = 240, width = 100, height = 100;

    // --- Output structure ---
    TgtData target = {0, Point(0, 0), Rect(), false};

    // --- Create black image ---
    Mat image = Mat::zeros(Size(IMG_WIDTH, IMG_HEIGHT), CV_8UC3);

    // --- Draw quadrant lines ---
    Point imgCenter(IMG_WIDTH / 2, IMG_HEIGHT / 2);
    line(image, Point(imgCenter.x, 0), Point(imgCenter.x, IMG_HEIGHT), Scalar(100, 100, 100), 1);
    line(image, Point(0, imgCenter.y), Point(IMG_WIDTH, imgCenter.y), Scalar(100, 100, 100), 1);

    // --- Draw bounding boxes for ClassID 0–3 ---
    for (auto &data : TgtDataArr) {
        if (data.ClassID <= 3) {
            rectangle(image, data.Box, Scalar(255, 255, 255), 1); // white box
            putText(image, to_string(data.ClassID), data.Centre + Point(-10, -10), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255), 1);
            circle(image, data.Centre, 3, Scalar(0, 255, 0), -1);
        }
    }

    // --- AOI Search ---
    bool found = false;
    int expandStep = 0;
    Rect AOI(X - width / 2, Y - height / 2, width, height);

    while (!found) {
        vector<AIData> candidates;

        // Clip AOI inside image bounds
        AOI &= Rect(0, 0, IMG_WIDTH, IMG_HEIGHT);

        // Collect valid detections within AOI
        for (auto &data : TgtDataArr) {
            if (data.ClassID > 3) continue; // only 0–3

            // Check if Flag == false → only consider 0 & 1
            if (!Flag && !(data.ClassID == 0 || data.ClassID == 1))
                continue;

            if (AOI.contains(data.Centre))
                candidates.push_back(data);
        }

        if (!candidates.empty()) {
            found = true;

            // Select closest to image center
            double minDist = 1e9;
            AIData best;

            for (auto &c : candidates) {
                double dist = norm(c.Centre - imgCenter);
                if (dist < minDist) {
                    minDist = dist;
                    best = c;
                }
            }

            target.ClassID = best.ClassID;
            target.Centre = best.Centre;
            target.Box = best.Box;
            target.ValidData = true;
            break;
        }

        // Expand AOI by 5px on all sides
        expandStep += 5;
        AOI.x = X - (width / 2 + expandStep);
        AOI.y = Y - (height / 2 + expandStep);
        AOI.width = width + expandStep * 2;
        AOI.height = height + expandStep * 2;

        // Stop if AOI covers entire image
        if (AOI.width >= IMG_WIDTH && AOI.height >= IMG_HEIGHT) {
            target.ValidData = false;
            break;
        }
    }

    // --- Draw AOI (final position) ---
    rectangle(image, AOI, Scalar(0, 0, 255), 2);

    // --- Draw target and connecting line ---
    if (target.ValidData) {
        rectangle(image, target.Box, Scalar(0, 255, 255), 1); // highlight target
        circle(image, target.Centre, 5, Scalar(0, 255, 255), -1);
        line(image, imgCenter, target.Centre, Scalar(255, 255, 0), 1);
        putText(image, "Target Class: " + to_string(target.ClassID),
                Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 1);
    } else {
        putText(image, "No Target Found", Point(10, 30),
                FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 0, 255), 2);
    }

    // --- Display Result ---
    imshow("Optimized AI Target Detection", image);
    waitKey(0);
    return 0;
}

#endif

