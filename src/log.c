#include "precomp.h"

#include "log.h"
#include "util.h"

typedef struct _tagLOGEVENTOBSERVER {
  LogObserverCallback callback;
  LPVOID userData;
  int id;
} LOGEVENTOBSERVER, *LPLOGEVENTOBSERVER;

typedef struct _tagLOGGER {
  PNTVECTOR(LPLOGENTRY) entries;
  PNTVECTOR(LPLOGEVENTOBSERVER) observers;
  int obsIDCounter;
} LOGGER, *LPLOGGER;

void LogMessage(int, LPWSTR, LPWSTR);

LPLOGGER GetLogger();
void Logger_PushMessage(LPLOGGER, LPLOGENTRY);

void LogMessage(int iType, LPWSTR szModule, LPWSTR szMessage)
{
  LOGENTRY logEntry = { 0 };

  GetSystemTime(&logEntry.timestamp);
  logEntry.iType = iType;
  StringCchCopy(logEntry.szModule, 80, szModule);
  StringCchCopy(logEntry.szMessage, 80, szMessage);

  LPLOGGER lpLogger = GetLogger();
  Logger_PushMessage(lpLogger, &logEntry);
}

LPLOGGER GetLogger()
{
  static LPLOGGER s_lpLogger = NULL;

  if (!s_lpLogger) {
    s_lpLogger = calloc(1, sizeof(LOGGER));
    PNTVECTOR_init(s_lpLogger->entries);
    PNTVECTOR_init(s_lpLogger->observers);
  }

  return s_lpLogger;
}

int LogRegisterObserver(LogObserverCallback callback, LPVOID userData)
{
  LPLOGGER lpLogger = GetLogger();
  assert(lpLogger);

  LPLOGEVENTOBSERVER lpLogObserver =
    (LPLOGEVENTOBSERVER) calloc(1, sizeof(LOGEVENTOBSERVER));
  lpLogObserver->callback = callback;
  lpLogObserver->userData = userData;
  lpLogObserver->id = lpLogger->obsIDCounter;

  PNTVECTOR_add(lpLogger->observers, lpLogObserver);

  return lpLogger->obsIDCounter++;
}

void Logger_PushMessage(LPLOGGER lpLogger, LPLOGENTRY lpEntry)
{
  LPLOGENTRY pMemEntry = calloc(1, sizeof(LOGENTRY));

  memcpy(pMemEntry, lpEntry, sizeof(LOGENTRY));
  PNTVECTOR_add(lpLogger->entries, pMemEntry);

  LOGEVENT logEvent;
  logEvent.iType = LOGALTEREVENT_ADD;
  logEvent.extra = PNTVECTOR_size(lpLogger->entries);

  for (size_t i = 0; i < PNTVECTOR_size(lpLogger->observers); ++i) {
    logEvent.userData = PNTVECTOR_at(lpLogger->observers, i)->userData;
    LogObserverCallback callback = PNTVECTOR_at(lpLogger->observers, i)->callback;
    (*callback)(&logEvent);
  }
}

void LogUnregisterObserver(int nID)
{
  LPLOGGER lpLogger = GetLogger();

  for (size_t i = 0; i < PNTVECTOR_size(lpLogger->observers); ++i) {

    if (PNTVECTOR_at(lpLogger->observers, i)->id == nID) {
      LPLOGEVENTOBSERVER lpObserver = PNTVECTOR_at(lpLogger->observers, i);

      free(lpObserver);
      PNTVECTOR_at(lpLogger->observers, i) = NULL;
      PNTVECTOR_remove(lpLogger->observers, i);
    }
  }
}

LPLOGENTRY RetrieveLogEntry(int i)
{
  LPLOGGER logger = GetLogger();

  if (i < 0 || i >= (int) PNTVECTOR_size(logger->entries))
    return NULL;

  return PNTVECTOR_at(logger->entries, i);
}

int LogGetSize()
{
  LPLOGGER logger = GetLogger();
  assert(logger);

  return PNTVECTOR_size(logger->entries);
}
