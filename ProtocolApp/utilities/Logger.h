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

#include "Times.h"

#define LOGGER Logger::getLogger()

enum class LOG_TYPE : int { LOG_ERROR = 0, LOG_WARNING, LOG_INFO };

extern std::mutex loggerMutex;

/*********************************************/
// Logging errors
/*********************************************/
void logError(const char * errorText);
/*********************************************/
// Logging info
/*********************************************/
void logInfo(const char * infoText);
/*********************************************/
// Logging warning
/*********************************************/
void logWarning(const char* infoText);


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
	std::ofstream logFile;
}; // class end

class LoggerDestroyer
{
	public:
	LoggerDestroyer();
	~LoggerDestroyer();
	void setLogger(Logger *s);

	private:
		Logger * m_logger = nullptr;
};