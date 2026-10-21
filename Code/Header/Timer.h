#pragma once

//C++
#include <chrono>
#include <ratio>

class Timer
{
public:

	Timer() 
	{
		Stop();
		Reset();
	}

	Timer(const Timer& sourceTimer) = default;
	Timer(Timer&& sourceTimer) noexcept = default;
	Timer& operator = (const Timer& sourceTimer) = default;
	Timer& operator = (Timer&& sourceTimer) noexcept = default;
	~Timer() = default;
	
	void Reset()
	{
		m_timePointStarted = Clock::now();
		m_timePointStopped = m_timePointStarted;
		m_timePointCurrent = m_timePointStarted;

		m_timePaused = Duration::zero();
		m_timeElapsed = Duration::zero();
	}

	void Stop()
	{
		if (m_isStopped == true)
		{
			return;
		}

		m_isStopped = true;

		m_timePointStopped = Clock::now();
	}

	void Start()
	{
		if (m_isStopped == false)
		{
			return;
		}

		m_isStopped = false;

		m_timePaused += Clock::now() - m_timePointStopped;
	}

	void Mark()
	{
		m_timePointStarted = Clock::now();
		m_timePointStopped = m_timePointStarted;
		m_timePointCurrent = m_timePointStarted;

		m_timePaused = Duration::zero();
	}

	void Update()
	{
		m_timePointCurrent = Clock::now();

		if (m_isStopped == false)
		{
			m_timeElapsed = (m_timePointCurrent - m_timePointStarted) - m_timePaused;
		}
		else
		{
			m_timeElapsed = (m_timePointStopped - m_timePointStarted) - m_timePaused;
		}
	}

	double GetTimeMilli() const
	{
		return DurationMilli(m_timeElapsed).count();
	}

private:

	using Clock			= std::chrono::steady_clock;
	using TimePoint		= std::chrono::time_point<std::chrono::steady_clock>;
	using Duration		= std::chrono::steady_clock::duration;
	using DurationMilli	= std::chrono::duration<double, std::milli>;

	bool m_isStopped;

	TimePoint m_timePointStarted;
	TimePoint m_timePointStopped;
	TimePoint m_timePointCurrent;

	Duration m_timePaused;
	Duration m_timeElapsed;
}; 