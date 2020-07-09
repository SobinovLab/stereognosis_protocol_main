/*********************************************************************
*
* Description:
*    This class manages the LOGGER
*
*********************************************************************/
#pragma once

#include <fstream>
#include <string>
#include <time.h>
#include <regex>
#include <mutex>
#include <memory>
#include <iostream>
#include <cstdio>

using namespace std;

#define LOGGER Logger::getLogger()

enum class LOG_TYPE : int { LOG_ERROR = 0, LOG_WARNING, LOG_INFO };

extern mutex loggerMutex;

/*********************************************/
// Logging errors
/*********************************************/
void logError(const char * errorText);
/*********************************************/
// Logging info
/*********************************************/
void logInfo(const char * infoText);

/*********************************************/
// Format log strings
/*********************************************/
template<typename ... Args>
string string_format(const std::string& format, Args ... args);

class Logger {

	public:
	/**
	*   Funtion to create the instance of logger class.
	*   @return singleton object of Logger class..
	*/
	static Logger* getLogger();
	/**
	*   Logs a type between LOG_ERROR - LOG_WARNING - LOG_INFO
	*   @param sMessage message to be logged.
	*/
    friend Logger& operator<<(Logger &logger, const LOG_TYPE logType);
    /**
	*   Logs a message
	*   @param text message to be logged.
	*/
    friend Logger& operator<<(Logger &logger, const char *text);
	/**
	*   Logs a end line 
	*/
	void endLine();
	/**
	*  Get current date/time with a formatted string
	**/
	static string currentDateTime(const char * formatStr);
	/**
	*  Get current date/time in milliseconds 2012-05-16 13:36:56:396
	**/
	static string currentDateTimeInMilliseconds();
	protected:
	/**
	*    Destructor for the Logger class.
	*/
	friend class LoggerDestroyer;
    virtual ~Logger();

	private:
	/**
	*    Default constructor for the Logger class.
	*/
		Logger();
	/**
	*   Make it Non Copyable (or you can inherit from sf::NonCopyable if you want)
	*/
		Logger(const Logger &);
	/**
	*   Singleton logger class object pointer.
	**/
	static Logger* m_instance;
	static LoggerDestroyer m_destroyer;
	/**
	*   Log file stream object.
	**/
	ofstream logFile;
    /**
	*   Warnings and errors statistics
	**/
	unsigned int m_numGoodTrials;
    unsigned int m_numTrialsAborted;
	unsigned int m_numTotalTrials;
}; // class end

class LoggerDestroyer
{
	public:
	LoggerDestroyer();
	~LoggerDestroyer();
	void setLogger(Logger *s);

	private:
		Logger * m_logger;
};