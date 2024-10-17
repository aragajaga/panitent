#ifndef PANITENT_GRIMSTROKE_PATH_H
#define PANITENT_GRIMSTROKE_PATH_H

#include "../util/tenttypes.h"

typedef struct PathCommand PathCommand;
typedef struct Path Path;

typedef enum {
    PATHCOMMAND_MOVETO,
    PATHCOMMAND_LINETO,
    PATHCOMMAND_CURVETO,
    PATHCOMMAND_ARCTO
} PathCommandType;

struct PathCommand {
    PathCommandType type;
    TENTPointf p1;
    TENTPointf p2;
    TENTPointf control;
    float radius;
    struct PathCommand* next;
};

struct Path {
    PathCommand* head;
    PathCommand* tail;
};

Path* Path_Create()
{
    Path* pPath = (Path*)malloc(sizeof(Path));
    pPath->head = NULL;
    pPath->tail = NULL;
    return pPath;
}

void Path_MoveTo(Path* path, float x, float y)
{
    PathCommand* cmd = (PathCommand*)malloc(sizeof(PathCommand));
    cmd->type = PATHCOMMAND_MOVETO;
    cmd->p1.x = x;
    cmd->p2.y = y;
    cmd->next = NULL;

    if (!path->head) {
        path->head = cmd;
    }
    else {
        path->tail->next = cmd;
    }
    path->tail = cmd;
}

void Path_LineTo(Path* path, float x, float y)
{
    PathCommand* cmd = (PathCommand*)malloc(sizeof(PathCommand));
    cmd->type = PATHCOMMAND_LINETO;
    cmd->p1.x = x;
    cmd->p2.y = y;
    cmd->next = NULL;

    if (!path->head) {
        path->head = cmd;
    }
    else {
        path->tail->next = cmd;
    }
    path->tail = cmd;
}

void Path_CurveTo(Path* path, float cx, float cy, float x, float y)
{
    PathCommand* cmd = (PathCommand*)malloc(sizeof(PathCommand));
    cmd->type = PATHCOMMAND_ARCTO;
    cmd->control.x = cx;
    cmd->control.y = cy;
    cmd->p1.x = x;
    cmd->p1.y = y;
    cmd->next = NULL;

    if (!path->head) {
        path->head = cmd;
    }
    else {
        path->tail->next = cmd;
    }
    path->tail = cmd;
}

void Path_ArcTo(Path* path, float cx, float cy, float radius, float startAngle, float endAngle)
{
    PathCommand* cmd = (PathCommand*)malloc(sizeof(PathCommand));
    cmd->type = PATHCOMMAND_ARCTO;
    cmd->p1.x = cx;
    cmd->p1.y = cy;
    cmd->radius = radius;
    cmd->p2.x = startAngle;
    cmd->p2.y = endAngle;
    cmd->next = NULL;

    if (!path->head) {
        path->head = cmd;
    }
    else {
        path->tail->next = cmd;
    }
    path->tail = cmd;
}

#endif  /* PANITENT_GRIMSTROKE_PATH_H */
