#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "timer.hpp"

// read frames with timestamps from stdin
// parse the timestamps and send them over udp at appropriate times
int main(int argc, char** argv)
{
    Tools::Timer* timer = Tools::Timer::createTimer();
    Tools::Timer* runTimer = Tools::Timer::createTimer();
    Tools::Timer* delayTimer = Tools::Timer::createTimer();
    int columns = 40;
    int rows = 25;
    float lastDisplayTime = -1;
    float time;
    uint8_t* data;
    char destAddr[256];
    int port = 9600;
    char tempFile[256];
    char ncCmdString[1024];
    int c;
    int framesPerSecond = 0;
    int lastSecond = 0;
    bool useTime = true;
    int maxRate = 6;
    double delayTime = 0;

    sprintf(destAddr, "127.0.0.1");
    sprintf(tempFile, "/mnt/tmp/temp.img");

    while ((c = getopt(argc, argv, "a:f:p:x:c:r:")) != -1)
    {
        if (c == 'a')
        {
            strcpy(destAddr, optarg);
        }
        else if (c == 'f')
        {
            strcpy(tempFile, optarg);
        }
        else if (c == 'p')
        {
            port = atoi(optarg);
        }
        else if (c == 'x')
        {
            useTime = false;
        }
        else if (c == 'c')
        {
            columns = atoi(optarg);
        }
        else if (c == 'r')
        {
            maxRate = atoi(optarg);
        }
    }

    int bytesInFrame = rows * columns;
    data = new uint8_t[bytesInFrame];

    // prepare command string
    sprintf(ncCmdString, "cat %s | nc -N %s %d", tempFile, destAddr, port);
    runTimer->start();
    delayTimer->start();

    if (maxRate > 0)
    {
        delayTime = 1.0 / (double)maxRate;
    }
    
    int framesSkipped = 0;

    while (1)
    {
        if (fread(&time, sizeof(float), 1, stdin) != 1)
        {
            break;
        }

        if (fread(data, 1, bytesInFrame, stdin) != bytesInFrame)
        {
            break;
        }

        // check the timing of this frame and wait if needed
        if (lastDisplayTime < 0 || time < lastDisplayTime)
        {
            // start the timer
            timer->start();
        }

        bool skip = false;
        if (useTime)
        {
            double timeNow = timer->getTime();
            if (timeNow > time + 1.0/30.0)
            {
                // skip frame
                skip = true;
            }
            else
            {
                while (timeNow < time)
                {
                    // sleep while frame is still early
                    usleep(100);
                    timeNow = timer->getTime();
                }
            }
        }
        lastDisplayTime = (double)time;

        if (skip)
        {
            framesSkipped++;
        }
        else
        {
            // check if we are running too fast and sleep if needed
            if (delayTime > 0) 
            {
                double elapsed = delayTimer->getTime();
                while (elapsed < delayTime)
                {
                    usleep(100);
                    elapsed = delayTimer->getTime();
                }
                delayTimer->start();
            }

            lastDisplayTime = (double)time;

            // write to file
            FILE* fp = fopen(tempFile, "wb");
            fwrite(data, 1, bytesInFrame, fp);
            fclose(fp);

            system(ncCmdString);
            framesPerSecond++;

            // display fps once per second
            double runTime = runTimer->getTime();
            if ((int)runTime > lastSecond)
            {
                lastSecond = (int)runTime;
                printf("%d FPS %d frames skipped\n", framesPerSecond, framesSkipped);
                framesPerSecond = 0;
                framesSkipped = 0;
            }
        }
    }
}