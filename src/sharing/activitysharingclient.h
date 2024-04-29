#pragma once

typedef struct ActivitySharingClient ActivitySharingClient;
struct ActivitySharingClient {
    void* m_handler;
    void (*SetStatusMessage)(void* handler, PCWSTR pszStatusMessage);
};
