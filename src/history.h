#ifndef PANITENT_HISTORY_H_
#define PANITENT_HISTORY_H_

typedef struct _Document Document;
typedef struct _Canvas Canvas;

typedef struct _HistoryRecord {
  struct _HistoryRecord *previous;
  struct _HistoryRecord *next;
  Canvas *differential;
  RECT rc;
} HistoryRecord;

typedef struct _History {
  HistoryRecord* peak;
  HistoryRecord* saved;
} History;

void History_Undo(Document* document);
void History_Redo(Document* document);
void History_StartDifferentiation(Document* document);
void History_FinalizeDifferentiation(Document* document);
void History_MarkSaved(Document* document);
BOOL History_IsDirty(Document* document);

#endif  /* PANITENT_HISTORY_H_ */
