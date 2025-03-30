// src/symbol/scopetree.h

#pragma once
#include "symbol.h"
#include "table.h"

Symbol* ScopeTreeFind(const SymbolTable* scope, char* name);
