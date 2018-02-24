#ifndef TIMER_H
#define TIMER_H

#include "FileSender.h"
#include "FileDownloader.h"
#include "Communication.h"

#include <pthread.h>
#include <sys/socket.h>
#include <time.h>

#define MAX_TRIES 	3
#define NO_SOC		-1

template<typename T>// Thanks @ https://isocpp.org/wiki/faq/pointers-to-members
class Timer
{
	public:
		Timer(int timeWait, T* obj, bool (T::*timeoutF)(), int socketToShutdown);
		~Timer();
		
		void stop(); // Call this if we got our data. 
		bool attemptsFinished(); // Check to see if we timed out all our attempts
		
	private:
		int timeWait; // This value is in seconds. 
		T* object;
		bool (T::*timeFunc)();
		int socketToShutdown = -1;
		
		int numTries = 0;
		bool running = true;
		pthread_t thread;
		pthread_mutex_t timerMutex;
		pthread_cond_t timerCond;
		
		void countDown();
		static void* startupTimerThread(void* timer);
};

// TEMPLATES MUST BE DEFINED IN THE HEADER

template <typename T>
Timer<T>::Timer(int timeWait, T* obj, bool (T::*timeoutF)(), int socketToShutdown):
timeWait(timeWait),
object(obj),
timeFunc(timeoutF),
socketToShutdown(socketToShutdown)
{
	pthread_mutex_init(&timerMutex, NULL);
	pthread_cond_init(&timerCond, NULL);
	
	int rv = pthread_create(&thread, NULL, startupTimerThread, this);
	if (rv != 0)
	{
		std::cout << "Error creating Timer thread, exiting program" << std::endl;
		exit(-1);
	}
}

template <typename T>
Timer<T>::~Timer()
{
	pthread_mutex_destroy(&timerMutex);
	pthread_cond_destroy(&timerCond);
}

template <typename T>
void Timer<T>::countDown()
{
	while(!attemptsFinished() && running)
	{
		pthread_mutex_lock(&timerMutex);
		{
			// Timing info, thanks @ https://www.ibm.com/support/knowledgecenter/en/ssw_i5_54/apis/users_77.htm
			struct timeval tp;
			struct timespec ts;
			
			gettimeofday(&tp, NULL); // Store the current time in the tp var
			
			/* Convert from timeval to timespec */
			ts.tv_sec  = tp.tv_sec;
			ts.tv_nsec = tp.tv_usec * 1000;
			ts.tv_sec += timeWait;
			
			while(running)
			{
				int rv = pthread_cond_timedwait(&timerCond, &timerMutex, &ts);
				
				if (rv == ETIMEDOUT) 
				{
					if((object->*timeFunc)())
						numTries++;
					else
						numTries = MAX_TRIES; // Force a time out due to error. 
					
					break;
				}
			}
		}
		pthread_mutex_unlock(&timerMutex);
	}
	
	if(attemptsFinished() && socketToShutdown != NO_SOC)
		shutdown(socketToShutdown, SHUT_RDWR);
}

template <typename T>
void Timer<T>::stop()
{
	pthread_mutex_lock(&timerMutex);
	{
		if(running)
		{
			running = false;
			pthread_cond_signal(&timerCond);	
		}
	}
	pthread_mutex_unlock(&timerMutex);
}

template <typename T>
bool Timer<T>::attemptsFinished()
{
	bool rv;
	
	pthread_mutex_lock(&timerMutex);
	{
		rv = numTries == MAX_TRIES;
	}
	pthread_mutex_unlock(&timerMutex);
	
	return rv;
}

template <typename T>
void* Timer<T>::startupTimerThread(void* timer)
{
	Timer* t = (Timer*)timer;
	t->countDown();
	
	delete t;
	return NULL;
}

#endif













