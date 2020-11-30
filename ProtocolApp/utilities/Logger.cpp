#include "Logger.h"

/**
* Date and time format strings
*/
#define DATE_TIME_FORMAT "%Y_%m_%d_%H_%M_%S"
#define ONLY_TIME_FORMAT "%H:%M:%S"
#define ONLY_DATE_FORMAT "%m/%d/%Y"
#define TIME_IN_MILLISECONDS "%02d:%02d:%02d.%03d"
/**
*   The name of the log file is structured so: 'filename'_date_time.'extension'
**/
#define LOG_FOLDER string("./data/")
#define LOG_FILE_NAME string("Protocol_")
#define LOG_EXTENSION string(".txt")

// Static variables
Logger* Logger::m_instance = NULL;
LoggerDestroyer Logger::m_destroyer;

mutex loggerMutex;

Logger* Logger::getLogger(){
	if (m_instance == NULL){
		m_instance = new Logger();
		m_destroyer.setLogger(m_instance);
	}
	return m_instance;
}

Logger::Logger():m_numGoodTrials(0U), m_numTrialsAborted(0U), m_numTotalTrials(0U)
{
	string fName = LOG_FOLDER + LOG_FILE_NAME;
	fName.append(currentDateTime(DATE_TIME_FORMAT));
	fName.append(LOG_EXTENSION);
	logFile.open(fName.c_str(), ios::out | ios::app);
    // Write the first lines
    if (logFile.is_open()) {
		logFile << "Log file created " << currentDateTime(ONLY_DATE_FORMAT).c_str() << std::endl << std::endl;
    } // if
}

// dtor
Logger::~Logger() {
	if (logFile.is_open()) {
        logFile << std::endl << std::endl;
        // Report number of errors and warnings
        logFile << m_numGoodTrials << " good trials" << std::endl;
        logFile << m_numTrialsAborted << " trials aborted" << std::endl;
		logFile << m_numTotalTrials << " total trials played" << std::endl;
        logFile.close();
    } // if
}

// Get current date/time, format is Wed May 30 12:25:03 2017
string Logger::currentDateTime(const char * formatStr) {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	localtime_s(&tstruct, &now);
	strftime(buf, sizeof(buf), formatStr, &tstruct);	
	return buf;
}

string Logger::currentDateTimeInMilliseconds()
{
	SYSTEMTIME stime;
	//structure to store system time (in usual time format)
	FILETIME ltime;
	//structure to store local time (local time in 64 bits)
	FILETIME ftTimeStamp;
	char TimeStamp[256];//to store TimeStamp information
	GetSystemTimeAsFileTime(&ftTimeStamp); //Gets the current system time

	FileTimeToLocalFileTime(&ftTimeStamp, &ltime);//convert in local time and store in ltime
	FileTimeToSystemTime(&ltime, &stime);//convert in system time and store in stime

	sprintf(TimeStamp, TIME_IN_MILLISECONDS, stime.wHour, stime.wMinute, stime.wSecond, stime.wMilliseconds);
	return string(TimeStamp);
}

void Logger::endLine() {
	logFile << std::endl;
}

// Overload << operator using log type
Logger& operator<<(Logger &logger, const LOG_TYPE logType) {
	logger.logFile << logger.currentDateTimeInMilliseconds().c_str();
	//logger.logFile << logger.currentDateTime(ONLY_TIME_FORMAT).c_str();
    switch (logType) {
		case LOG_TYPE::LOG_ERROR:
            logger.logFile << "[ABORTED]: ";
            ++logger.m_numTrialsAborted;
            break;
		case LOG_TYPE::LOG_WARNING:
            logger.logFile << "[WARNING]: ";
            break;
		case LOG_TYPE::LOG_INFO:
			logger.logFile << "[GOOD]: "; 
			++logger.m_numGoodTrials;
			break;
        default:
            logger.logFile << "LOGGER error: LogType has not been recognized";
            break;
    } // sw
	++logger.m_numTotalTrials;
    return logger;
}

// Overload << operator using C style strings
// No need for std::string objects here
Logger& operator<<(Logger &logger, const char *text) {
	logger.logFile << text;
    return logger;
}

LoggerDestroyer::LoggerDestroyer()
{
}

LoggerDestroyer::~LoggerDestroyer()
{
	delete m_logger->m_instance;
}

void LoggerDestroyer::setLogger(Logger *s)
{
	m_logger = s;
}

void logError(const char * errorText) {
	Logger * logger = LOGGER;
	loggerMutex.lock();
	*logger << LOG_TYPE::LOG_ERROR << errorText;
	logger->endLine();
	loggerMutex.unlock();
}

void logInfo(const char * infoText) {
	Logger * logger = LOGGER;
	loggerMutex.lock();
	*logger << LOG_TYPE::LOG_INFO << infoText;
	logger->endLine();
	loggerMutex.unlock();
}

template<typename ... Args>
string string_format(const std::string& format, Args ... args)
{
	size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
	unique_ptr<char[]> buf(new char[size]);
	snprintf(buf.get(), size, format.c_str(), args ...);
	return string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}