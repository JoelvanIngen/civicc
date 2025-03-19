// src/symbol/scopetree.h

#pragma once
#include "common.h"
#include "symbol.h"
#include "table.h"

Symbol* ScopeTreeFind(SymbolTable* scope, char* name);
