#ifndef TIMER_H
#define TIMER_H

#include "FileSender.h"
#include "FileDownloader.h"
#include "Communication.h"

#include <pthread.h>
#include <sys/socket.h>
#include <time.h>

#define MAX_TRIES 	3
#define WAIT_TIME	3
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
	while(running && !attemptsFinished())
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
				
				// Need another running check here as lets say the stack frame holding object completes its. When this happens timer.stop is called, but what if this the stack frame is destroyed before this thread
				// gets the signal, than the call to object->* will be invalid as that memory is gone. 
				if(rv == ETIMEDOUT && running)
				{
					if((object->*timeFunc)())
						numTries++;
					else
						numTries = MAX_TRIES; // Force a time out due to error. 
					
					break; // If it was a timeout, and not a spurious wakeup, we break so we can remake the time. 
				}
			}
		}
		pthread_mutex_unlock(&timerMutex);
	}
	
	// This is a timeout finish, it will never occur due to stop() being called. 
	if(running && attemptsFinished() && socketToShutdown != NO_SOC)
	{
		running = false;
		shutdown(socketToShutdown, SHUT_RDWR);
	}
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
	t->stop();
	
	delete t;
	return NULL;
}

#endif













