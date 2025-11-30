#ifndef HEADER_H
#define HEADER_H

typedef struct Header_ Header;

Header* Header_new(const char* title);
void Header_delete(Header* this);
void Header_setTitle(Header* this, const char* title);
void Header_draw(const Header* this);
void Header_setStatus(Header* this, const char* status);
void Header_setRuntime(Header* this, double runtime);

#endif