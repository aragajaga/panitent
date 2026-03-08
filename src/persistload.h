#pragma once

typedef enum PersistLoadStatus
{
	PERSIST_LOAD_OK = 0,
	PERSIST_LOAD_NOT_FOUND,
	PERSIST_LOAD_INVALID_FORMAT,
	PERSIST_LOAD_IO_ERROR
} PersistLoadStatus;
