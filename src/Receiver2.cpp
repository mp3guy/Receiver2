
#include "TcpReceiver.h"

int find_argument(int argc, char** argv, const char* argument_name)
{
    for(int i = 1; i < argc; ++i)
    {
        if(strcmp(argv[i], argument_name) == 0)
        {
            return i;
        }
    }
    return -1;
}

int parse_argument(int argc, char** argv, const char* str, int &val)
{
    int index = find_argument (argc, argv, str) + 1;

    if (index > 0 && index < argc )
      val = atoi (argv[index]);

    return (index - 1);
}

int parse_argument(int argc, char** argv, const char* str, std::string &val)
{
    int index = find_argument (argc, argv, str) + 1;
    
    if (index > 0 && index < argc )
      val = argv[index];

    return index - 1;
}

int main(int argc, char * argv[])
{
    int width = 640;
    int height = 480;

    parse_argument(argc, argv, "-w", width);
    parse_argument(argc, argv, "-h", height);

    std::string ipAddr;
    assert(parse_argument(argc, argv, "-a", ipAddr) > 0 && "Please provide a host");

    bool drawImages = false;

    drawImages = (find_argument(argc, argv, "-v") != -1);

    std::vector<unsigned long long int> frameStats;
    
    TcpReceiver tcp(ipAddr, "5698", width, height);

    while(true)
    {
        if(tcp.receive())
        {
            frameStats.push_back(tcp.timestamp);

            if(frameStats.size() > 30)
            {
                frameStats.erase(frameStats.begin());
            }

            float timeSum = 0;

            for(unsigned int i = 1; i < frameStats.size(); i++)
            {
                timeSum += frameStats.at(i) - frameStats.at(i - 1);
            }

            if(frameStats.size() > 1)
            {
                timeSum /= frameStats.size() - 1;
            }

            std::stringstream fpsText;
            fpsText << "Framerate: " << 1.0 / (timeSum / 1000000.0) << "fps   ";

            if(drawImages)
            {
                cv::Mat3b rgb(height, width, (cv::Vec<unsigned char, 3> *)tcp.rgb, width * 3);
                cv::cvtColor(rgb, rgb, CV_RGB2BGR);
                cv::putText(rgb, fpsText.str(), cv::Point(10, 20), CV_FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0, 255, 0));

                imshow("rgb", rgb);

                cv::Mat1w depth(height, width, tcp.depth);
                cv::Mat1b tmp;
                normalize(depth, tmp, 0, 255, cv::NORM_MINMAX, 0);
                cv::putText(tmp, fpsText.str(), cv::Point(20, 20), CV_FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0, 255, 0));

                imshow("depth", tmp);

                cvWaitKey(1);
            }
            else
            {
                std::cout << "\r" << fpsText.str();
                std::cout.flush();
            }
        }
    }

    return 0;
}

