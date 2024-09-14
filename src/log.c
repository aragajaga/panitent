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

void LogMessage(int nLevel, PCWSTR pszModule, PCWSTR pszMessage)
{
  LOGENTRY logEntry = { 0 };

  GetSystemTime(&logEntry.timestamp);
  logEntry.iType = nLevel;

  int len = wcslen(pszModule);
  size_t size = (len + 1) * sizeof(WCHAR);
  logEntry.pszModule = (PWSTR)malloc(size);
  if (logEntry.pszModule)
  {
      memset(logEntry.pszModule, 0, size);
      wcscpy_s(logEntry.pszModule, len + 1, pszModule);
  }
  else {
      Panitent_RaiseException(L"Memory allocation fault");
      return;
  }

  len = wcslen(pszMessage);
  size = (len + 1) * sizeof(WCHAR);
  logEntry.pszMessage = (PWSTR)malloc(size);
  if (logEntry.pszMessage)
  {
      memset(logEntry.pszMessage, 0, size);
      wcscpy_s(logEntry.pszMessage, len + 1, pszMessage);
  }
  else {
      Panitent_RaiseException(L"Memory allocation fault");
      return;
  }

  LPLOGGER lpLogger = GetLogger();
  Logger_PushMessage(lpLogger, &logEntry);
}

void LogMessageF(int nLevel, PCWSTR pszModule, PCWSTR pszFormat, ...)
{
    va_list argp, argp2;
    va_start(argp, pszFormat);
    va_copy(argp2, argp);

    int len = vswprintf(NULL, 0, pszFormat, argp2);
    size_t size = (len + 1) * sizeof(WCHAR);
    PWSTR pszString = (PWSTR)malloc(size);
    if (pszString)
    {
        memset(pszString, 0, size);
        vswprintf_s(pszString, len + 1, pszFormat, argp);

        LogMessage(nLevel, pszModule, pszString);

        free(pszString);
    }
    else {
        Panitent_RaiseException(L"Memory allocation fault");
    }

    va_end(argp);
    va_end(argp2);
}

LPLOGGER GetLogger()
{
  static LPLOGGER s_lpLogger = NULL;

  if (!s_lpLogger) {
    s_lpLogger = (LPLOGGER)malloc(sizeof(LOGGER));
    if (s_lpLogger)
    {
        memset(s_lpLogger, 0, sizeof(LOGGER));
        pntvector_init(LPLOGENTRY)(&s_lpLogger->entries);
        pntvector_init(LPLOGEVENTOBSERVER)(&s_lpLogger->observers);
    }
    else {
        Panitent_RaiseException(L"Memory allocation fault");
    }
  }

  return s_lpLogger;
}

int LogRegisterObserver(LogObserverCallback callback, LPVOID userData)
{
  LPLOGGER lpLogger = GetLogger();
  assert(lpLogger);

  LPLOGEVENTOBSERVER pObserver = (LPLOGEVENTOBSERVER)malloc(sizeof(LOGEVENTOBSERVER));
  if (pObserver)
  {
      memset(pObserver, 0, sizeof(LOGEVENTOBSERVER));

      pObserver->callback = callback;
      pObserver->userData = userData;
      pObserver->id = lpLogger->obsIDCounter;

      pntvector_add(LPLOGEVENTOBSERVER)(&lpLogger->observers, pObserver);

      return lpLogger->obsIDCounter++;
  }
  else {
      Panitent_RaiseException(L"Memory allocation fault");
  }

  return 0;
}

void Logger_BroadcastEvent(LPLOGGER pLogger, int iType, int extra, void* pData)
{
    LOGEVENT logEvent;
    logEvent.iType = iType;
    logEvent.userData = pData;
    logEvent.extra = extra;

    for (size_t i = 0; i < pntvector_size(LPLOGEVENTOBSERVER)(&pLogger->observers); ++i)
    {
        logEvent.userData = pntvector_at(LPLOGEVENTOBSERVER)(&pLogger->observers, i)->userData;
        LogObserverCallback callback = pntvector_at(LPLOGEVENTOBSERVER)(&pLogger->observers, i)->callback;
        (*callback)(&logEvent);
    }
}

void Logger_PushMessage(LPLOGGER lpLogger, LPLOGENTRY lpEntry)
{
  LPLOGENTRY pSharedLogEntry = (LPLOGENTRY)malloc(sizeof(LOGENTRY));
  if (pSharedLogEntry)
  {
      memcpy(pSharedLogEntry, lpEntry, sizeof(LOGENTRY));
      pntvector_add(LPLOGENTRY)(&lpLogger->entries, pSharedLogEntry);
  }

  Logger_BroadcastEvent(lpLogger, LOGALTEREVENT_ADD, (int)pntvector_size(LPLOGENTRY)(&lpLogger->entries), NULL);
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

  if (i < 0 || i >= (int)pntvector_size(LPLOGENTRY)(&logger->entries))
  {
      return NULL;
  }

  return pntvector_at(LPLOGENTRY)(&logger->entries, i);
}

int LogGetSize()
{
  LPLOGGER logger = GetLogger();
  assert(logger);

  return (int) pntvector_size(LPLOGENTRY)(&logger->entries);
}

void LogEntry_Destroy(LPLOGENTRY pLogEntry)
{
    pLogEntry->iType = 0;
    memset(&pLogEntry->timestamp, 0, sizeof(SYSTEMTIME));
    free(pLogEntry->pszModule);
    free(pLogEntry->pszMessage);
}

void ClearLog()
{
    LPLOGGER pLogger = GetLogger();
    assert(pLogger);

    Logger_BroadcastEvent(pLogger, LOGALTEREVENT_CLEAR, 0, NULL);

    int nLogEntries = LogGetSize();
    for (unsigned int i = 0; i < nLogEntries; ++i)
    {
        LPLOGENTRY pSharedLogEntry = RetrieveLogEntry(i);
        LogEntry_Destroy(pSharedLogEntry);
        free(pSharedLogEntry);
    }

    int nVecSize = pntvector_size(LPLOGENTRY)(&pLogger->entries);
    for (int i = 0; i < nVecSize; ++i)
    {
        pntvector_remove(LPLOGENTRY)(&pLogger->entries, 0);
    }   
}
