#include "precomp.h"
#include "log.h"
#include "util.h"

typedef struct _tagLOGEVENTOBSERVER LOGEVENTOBSERVER, *LPLOGEVENTOBSERVER;

PNTVECTOR_DECLARE_TYPE(LPLOGENTRY)
PNTVECTOR_DECLARE_TYPE(LPLOGEVENTOBSERVER)

struct _tagLOGEVENTOBSERVER {
  LogObserverCallback callback;
  LPVOID userData;
  int id;
};

typedef struct _tagLOGGER {
  pntvector(LPLOGENTRY) entries;
  pntvector(LPLOGEVENTOBSERVER) observers;
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
    pntvector_init(LPLOGENTRY)(&s_lpLogger->entries);
    pntvector_init(LPLOGEVENTOBSERVER)(&s_lpLogger->observers);
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

  pntvector_add(LPLOGEVENTOBSERVER)(&lpLogger->observers, lpLogObserver);

  return lpLogger->obsIDCounter++;
}

void Logger_PushMessage(LPLOGGER lpLogger, LPLOGENTRY lpEntry)
{
  LPLOGENTRY pMemEntry = calloc(1, sizeof(LOGENTRY));

  memcpy(pMemEntry, lpEntry, sizeof(LOGENTRY));
  pntvector_add(LPLOGENTRY)(&lpLogger->entries, pMemEntry);

  LOGEVENT logEvent;
  logEvent.iType = LOGALTEREVENT_ADD;
  logEvent.extra = (int) pntvector_size(LPLOGENTRY)(&lpLogger->entries);

  for (size_t i = 0; i < pntvector_size(LPLOGEVENTOBSERVER)(&lpLogger->observers); ++i) {
    logEvent.userData = pntvector_at(LPLOGEVENTOBSERVER)(&lpLogger->observers, i)->userData;
    LogObserverCallback callback = pntvector_at(LPLOGEVENTOBSERVER)(&lpLogger->observers, i)->callback;
    (*callback)(&logEvent);
  }
}

void LogUnregisterObserver(int nID)
{
  LPLOGGER lpLogger = GetLogger();

  for (size_t i = 0; i < pntvector_size(LPLOGEVENTOBSERVER)(&lpLogger->observers); ++i) {

    if (pntvector_at(LPLOGEVENTOBSERVER)(&lpLogger->observers, i)->id == nID) {
      LPLOGEVENTOBSERVER lpObserver = pntvector_at(LPLOGEVENTOBSERVER)(&lpLogger->observers, i);

      free(lpObserver);
      pntvector_set(LPLOGEVENTOBSERVER)(&lpLogger->observers, i, NULL);
      pntvector_remove(LPLOGEVENTOBSERVER)(&lpLogger->observers, i);
    }
  }
}

LPLOGENTRY RetrieveLogEntry(int i)
{
  LPLOGGER logger = GetLogger();

  if (i < 0 || i >= (int) pntvector_size(LPLOGENTRY)(&logger->entries))
    return NULL;

  return pntvector_at(LPLOGENTRY)(&logger->entries, i);
}

int LogGetSize()
{
  LPLOGGER logger = GetLogger();
  assert(logger);

  return (int) pntvector_size(LPLOGENTRY)(&logger->entries);
}
