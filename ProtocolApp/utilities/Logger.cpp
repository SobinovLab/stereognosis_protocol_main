#include "Logger.h"

using namespace std;

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

Logger::Logger()
{
	string fName = LOG_FOLDER + LOG_FILE_NAME;
	CreateDirectory(LOG_FOLDER.c_str(), NULL);
	fName.append(Times::getFormattedDateTime());
	fName.append(LOG_EXTENSION);

	logFile.open(fName.c_str(), ios::out | ios::app);

    // Write the first lines
    if (logFile.is_open()) {
		logFile << "Log file created " << Times::getFormattedDate() << std::endl << std::endl;
    } // if
}

// dtor
Logger::~Logger() {
	if (logFile.is_open()) {
        logFile << std::endl << std::endl;
        logFile.close();
    } // if
}

void Logger::endLine() {
	logFile << std::endl;
}

// Overload << operator using log type
Logger& operator<<(Logger &logger, const LOG_TYPE logType) {
	logger.logFile << Times::getCurrentTimeInMilliSecs();
    switch (logType) {
		case LOG_TYPE::LOG_ERROR:
            logger.logFile << "[ERROR]: ";
            break;
		case LOG_TYPE::LOG_WARNING:
            logger.logFile << "[WARNING]: ";
            break;
		case LOG_TYPE::LOG_INFO:
			logger.logFile << "[INFO]: "; 
			break;
        default:
            logger.logFile << "[UNKNOWNTYPE]: ";
            break;
    } // sw
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
	if (m_logger)
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

void logWarning(const char* infoText) {
	Logger* logger = LOGGER;
	loggerMutex.lock();
	*logger << LOG_TYPE::LOG_WARNING << infoText;
	logger->endLine();
	loggerMutex.unlock();
}