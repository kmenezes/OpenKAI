/*
 * ThreadBase.cpp
 *
 *  Created on: Aug 20, 2015
 *      Author: yankai
 */

#include "_ThreadBase.h"

namespace kai
{

_ThreadBase::_ThreadBase()
{
	m_bThreadON = false;
	m_threadID = 0;
	m_timeStamp = 0;
	m_dTime = 1.0;
	m_FPS = 0;
	m_targetFPS = DEFAULT_FPS;
	m_targetFrameTime = USEC_1SEC / m_targetFPS;
	m_timeFrom = 0;
	m_timeTo = 0;
	m_bSleep = false;

	pthread_mutex_init(&m_wakeupMutex, NULL);
	pthread_cond_init(&m_wakeupSignal, NULL);
}

_ThreadBase::~_ThreadBase()
{
	pthread_mutex_destroy(&m_wakeupMutex);
	pthread_cond_destroy(&m_wakeupSignal);
}

bool _ThreadBase::init(void* pKiss)
{
	IF_F(!this->BASE::init(pKiss));
	Kiss* pK = (Kiss*) pKiss;

	int FPS = DEFAULT_FPS;
	F_INFO(pK->v("FPS", &FPS));
	setTargetFPS(FPS);

	return true;
}

bool _ThreadBase::link(void)
{
	IF_F(!this->BASE::link());
	return true;
}

bool _ThreadBase::start(void)
{
	return true;
}

void _ThreadBase::sleepTime(int64_t usec)
{
	if(usec>0)
	{
		struct timeval now;
		struct timespec timeout;

		gettimeofday(&now, NULL);
		int64_t nsec = (now.tv_usec + usec) * 1000;
		timeout.tv_sec = now.tv_sec + nsec / NSEC_1SEC;
		timeout.tv_nsec = nsec % NSEC_1SEC;

		pthread_mutex_lock(&m_wakeupMutex);
		pthread_cond_timedwait(&m_wakeupSignal, &m_wakeupMutex, &timeout);
		pthread_mutex_unlock(&m_wakeupMutex);
	}
	else
	{
		pthread_mutex_lock(&m_wakeupMutex);
		pthread_cond_wait(&m_wakeupSignal, &m_wakeupMutex);
		pthread_mutex_unlock(&m_wakeupMutex);
	}

}

void _ThreadBase::sleep(void)
{
	m_bSleep = true;
}

void _ThreadBase::wakeUp(void)
{
	IF_(!m_bSleep);
	m_bSleep = false;
	pthread_cond_signal(&m_wakeupSignal);
}

void _ThreadBase::updateTime(void)
{
	uint64_t newTime = get_time_usec();
	m_dTime = newTime - m_timeStamp;
	m_timeStamp = newTime;
	m_FPS = USEC_1SEC / m_dTime;
}

double _ThreadBase::getFrameRate(void)
{
	return m_FPS;
}

void _ThreadBase::setTargetFPS(int fps)
{
	IF_(fps<=0);

	m_targetFPS = fps;
	m_targetFrameTime = USEC_1SEC / m_targetFPS;
}

void _ThreadBase::autoFPSfrom(void)
{
	m_timeFrom = get_time_usec();
}

void _ThreadBase::autoFPSto(void)
{
	m_timeTo = get_time_usec();

	int uSleep = (int) (m_targetFrameTime - (m_timeTo - m_timeFrom));
	if (uSleep > 1000)
	{
		this->sleepTime(uSleep);
	}

	if(m_bSleep)
	{
		m_FPS = 0;
		this->sleepTime(0);
	}

	this->updateTime();
}

void _ThreadBase::complete(void)
{
	m_bThreadON = false;
	pthread_cancel(m_threadID);
	pthread_join(m_threadID, NULL);
}

bool _ThreadBase::draw(void)
{
	IF_F(!this->BASE::draw());

	Window* pWin = (Window*)this->m_pWindow;
	Mat* pMat = pWin->getFrame()->getCMat();
	pWin->tabReset();

	string msg = *this->getName() + " FPS: " + i2str(m_FPS);
	pWin->addMsg(&msg);

	return true;
}

}
