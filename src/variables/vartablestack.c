// src/variables/vartablestack.c

#include "vartablestack.h"

#include "common.h"
#include "variable.h"
#include "palm/hash_table.h"
#include "palm/memory.h"

#define VTS VarTableStack           // Makes some definitions shorter

/**
 * Creates a new VTS
 * @return a pointer to a new variable table stack
 */
VTS * VTSnew() {
    VTS* vts = MEMmalloc(sizeof(VTS));
    vts->current = 0;
    return vts;
}

/**
 * Destroys the VTS and calls destructors on each table it contains
 * @param vts the VTS to destroy
 */
void VTSdestroy(VTS* vts) {
#ifdef DEBUGGING
    ASSERT_MSG((vts != NULL), "vts argument is NULL");
#endif // DEBUGGING
    // Delete all tables
    while (vts->current > 0) {
        VTSpop(vts);
    }
    // Delete stack
    MEMfree(vts);
}

/**
 * Stacks a new, empty table on the stack
 * @param vts the VTS to push a new table onto
 */
void VTSpush(VTS* vts) {
#ifdef DEBUGGING
    ASSERT_MSG((vts != NULL), "vts argument is NULL");
#endif // DEBUGGING
    if (vts->current + 1 >= VARTABLE_STACK_SIZE) {
        USER_ERROR("Stack overflow, scope depth reached past maximum of %i",
            VARTABLE_STACK_SIZE);
    }

    vts->stack[vts->current - 1] = HTnew_String(VARTABLE_SIZE);
    vts->current++;
}

/**
 * Destroys the uppermost table on the VTS
 * @param vts the VTS table to pop from
 */
void VTSpop(VTS* vts) {
#ifdef DEBUGGING
    ASSERT_MSG((vts != NULL), "vts argument is NULL");
    ASSERT_MSG((vts->current > 0),
        "Stack pointer needs to be greater than 0 to pop but is %i",
        vts->current);
#endif // DEBUGGING
    vts->current--;
    HTdelete(vts->stack[vts->current - 1]);
    vts->stack[vts->current - 1] = NULL;
}

/**
 * Returns a vartable on a specific index
 * @param vts the VTS table to peek on
 * @param height the index of the vartable to return
 * @return the requested vartable
 */
static htable_st* peek(const VTS* vts, const int height) {
#ifdef DEBUGGING
    ASSERT_MSG((vts != NULL), "vts argument is NULL");
    ASSERT_MSG((height >= 0), "height must be greater than or equal to 0, but was %i", height);
    ASSERT_MSG((height < VARTABLE_STACK_SIZE),
        "height (%i) is equal to or greater than table stack size (%i)", height, VARTABLE_STACK_SIZE);
#endif // DEBUGGING
    return vts->stack[height];
}

/**
 * Attempts to find a string (var/fn name) in the VTS stack, and returns the uppermost found
 * entry, or returns NULL if none are found
 * @param vts the VTS to look in
 * @param key the key to look up
 * @return the uppermost SV entry belonging to the name or NULL if failed
 */
ScopeValue* VTSfind(const VTS* vts, char* key) {
    for (int depth = vts->current - 1; depth >= 0; depth--) {
        htable_st* t = peek(vts, depth);
        ScopeValue* sv = HTlookup(t, key);
        if (sv != NULL) return sv;
    }

    return NULL;
}

/**
 * Adds a name and SV to the current vartable. Removes the old value if name already exists.
 * @param vts VTS to add to
 * @param key name of the variable or function
 * @param value SV belonging to the variable or function
 */
void VTSadd(VTS* vts, char* key, ScopeValue* value) {
    htable_st* t = peek(vts, vts->current - 1);

    // Ensure adding a new name removes the old entry first
    // TODO: this is probably going to leak memory - fix
    HTremove(t, key);

    HTinsert(t, key, value);
}
