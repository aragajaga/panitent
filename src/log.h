#ifndef PANITENT_LOG_H
#define PANITENT_LOG_H

typedef struct _tagLOGEVENT {
  int iType;
  int extra;
  LPVOID userData;
} LOGEVENT, *LPLOGEVENT;

typedef void (*LogObserverCallback)(LPLOGEVENT);

typedef struct _tagLOGENTRY {
  SYSTEMTIME timestamp;
  int iType;
  PWSTR pszModule;
  PWSTR pszMessage;
} LOGENTRY, *LPLOGENTRY;

enum {
  LOGALTEREVENT_ADD = 1,
  LOGALTEREVENT_DELETE,
  LOGALTEREVENT_CLEAR
};

enum {
  LOGENTRY_TYPE_DEBUG,
  LOGENTRY_TYPE_INFO,
  LOGENTRY_TYPE_WARNING,
  LOGENTRY_TYPE_ERROR
};

void ClearLog();
void LogMessage(int nLevel, PCWSTR pszModule, PCWSTR pszMessage);
void LogMessageF(int nLevel, PCWSTR pszModule, PCWSTR pszFormat, ...);
int LogRegisterObserver(LogObserverCallback, LPVOID);
void LogUnregisterObserver(int);
int LogGetSize();
LPLOGENTRY RetrieveLogEntry(int);

#endif  /* PANITENT_LOG_H */
