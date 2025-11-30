#ifndef EXPML_FUNCTIONBAR_H
#define EXPML_FUNCTIONBAR_H

typedef struct FunctionBar_ FunctionBar;
FunctionBar* FunctionBar_new(const char* const* keys, const char* const* labels);
void FunctionBar_delete(FunctionBar* this);
void FunctionBar_setContext(FunctionBar* this, const char* fmt, ...);
void FunctionBar_draw(const FunctionBar* this, int width);

#endif