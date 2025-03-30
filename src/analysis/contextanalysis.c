/**
 * @file
 *
 * Traversal: ContextAnalysis
 * UID      : CTA
 *
 *
 */

#include "ccn/ccn.h"
#include "ccngen/ast.h"
#include "ccngen/trav.h"

#include "common.h"
#include "global/globals.h"
#include "symbol/scopetree.h"
#include "symbol/table.h"

typedef enum {
    DECLARATION_PASS,
    ANALYSIS_PASS,
} PassType;

typedef enum Origin {
    GLOBAL_ORIGIN,
    LOCAL_ORIGIN,
    IMPORTED_ORIGIN,
} Origin;

static PassType PASS;
static ValueType LAST_TYPE;

// Current scope in the tree
static SymbolTable* CURRENT_SCOPE;

// Flag to determine if we are saving indexes to stack
static bool INDEXING_ARRAY = false;

// Keeps track of whether we had an error during analysis
static bool HAD_ERROR = false;

// Keeps track of missing return statements
static bool HAD_RETURN = false;

static size_t GLOBAL_VAR_OFFSET = 0;
static size_t FUN_IMPORT_OFFSET = 0;
static size_t VAR_IMPORT_OFFSET = 0;
static size_t FUN_EXPORT_OFFSET = 0;

#define IS_ARITH_TYPE(vt) (vt == VT_NUM || vt == VT_FLOAT)

// Short for printing error on duplicate identifier name and immediately returning
#define HANDLE_DUPLICATE_ID(name) do { \
    if (name_exists_in_top_scope(name)) { \
        HAD_ERROR = true; \
        USER_ERROR("Variable '%s' is already declared", name); \
        return node; \
    } \
} while (false)

#define HANDLE_MISSING_SYMBOL(name, s) do { \
    if (s == NULL) { \
        HAD_ERROR = true; \
        USER_ERROR("Variable '%s' has not been declared yet", name); \
        return node; \
    } \
} while (false)

/**
 * Exits if one or more errors have occurred
 */
static void exit_if_error() {
    if (HAD_ERROR) {
        USER_ERROR("One or multiple errors occurred, exiting...");
        exit(1);
    }
}

/**
 * Retrieves a given identifier in the top scope (not any parent scopes)
 * and returns existence of identifier
 * @param name name to lookup
 * @return boolean indicating presence of the name in the top scope
 */
static bool name_exists_in_top_scope(char* name) {
    return STlookup(CURRENT_SCOPE, name) != NULL;
}

/**
 * Counts the amount of expressions starting from an exprs node.
 * In case an exprs node points to an array, also counts the amount
 * of dimensions, since they will be pushed too in a function call
 * @param exprs_node starting exprs node
 * @return amount of expressions found
 */
static size_t count_exprs(node_st* exprs_node) {
    size_t count = 0;

    while (exprs_node != NULL) {
        count++;
        if (NODE_TYPE(EXPRS_EXPR(exprs_node)) == NT_VAR) {
            node_st* var_node = EXPRS_EXPR(exprs_node);
            Symbol* var_symbol = ScopeTreeFind(CURRENT_SCOPE, VAR_NAME(var_node));
            if (var_symbol == NULL) {
                HAD_ERROR = true;
                ERROR("Cannot find variable named %s", VAR_NAME(var_node));
            }

            if (IS_ARRAY(var_symbol->vtype)) {
                // Only add indices as parameters if array is not indexed
                if (VAR_INDICES(var_node) == NULL) {
                    count += var_symbol->as.array.dim_count;
                }
            }
        }
        exprs_node = EXPRS_NEXT(exprs_node);
    }

    return count;
}

/**
 * Counts the amount of IDs in a linked IDs list
 * @param ids_node starting ids node
 * @return amount of IDs found
 */
static size_t count_ids(node_st* ids_node) {
    size_t count = 0;

    while (ids_node != NULL) {
        count++;
        ids_node = IDS_NEXT(ids_node);
    }
    return count;
}

/**
 * Counts the amount of parameters. For any array, its dimension identifiers are also counted as a parameter.
 * @param param_node starting parameter node
 * @return amount of parameters
 */
static size_t count_params(node_st* param_node) {
    size_t count = 0;

    while (param_node != NULL) {
        count++;
        if (PARAM_DIMS(param_node) != NULL) count += count_ids(PARAM_DIMS(param_node));
        param_node = PARAM_NEXT(param_node);
    }

    return count;
}

/**
 * Creates symbols for all IDs and returns a pointer to an array of these symbols.
 * @param id_node starting ids node
 * @param count amount of IDs counted
 * @param orig origin of the array; imported, local
 * @return IDs as symbols with their own offset
 */
static Symbol** get_ids(node_st* id_node, const size_t count, const Origin orig) {
    Symbol** ids = MEMmalloc(sizeof(Symbol*) * count);

    for (size_t i = 0; i < count; i++) {
        char* name = IDS_NAME(id_node);
        Symbol* s = SBfromVar(name, VT_NUM, orig == IMPORTED_ORIGIN);
        ids[i] = s;

        if (STlookup(CURRENT_SCOPE, name)) {
            HAD_ERROR = true;
            USER_ERROR("Name %s was already defined within scope, but is defined again", name);
        }
        STinsert(CURRENT_SCOPE, name, s);

        switch (orig) {
            case IMPORTED_ORIGIN: s->offset = VAR_IMPORT_OFFSET++; break;
            case LOCAL_ORIGIN: s->offset = CURRENT_SCOPE->localvar_offset_counter++; break;
            default:
#ifdef DEBUGGING
                ERROR("Cannot get IDs on a global-defined variable");
#endif // DEBUGGING
        }

        id_node = IDS_NEXT(id_node);
    }

    return ids;
}

/**
 * Creates symbols for all expressions (dimensions) of array and returns pointer to
 * an array of these symbols
 * @param exprs_node starting exprs node
 * @param array_name name of the array used for the creating of symbol names
 * @param count amount of exprs expected
 * @param orig origin of the array; global or local
 * @return pointer to array of created symbols for array indices
 */
static Symbol** get_exprs(node_st* exprs_node, const char* array_name, const size_t count, const Origin orig) {

    Symbol** ids = MEMmalloc(sizeof(Symbol*) * count);

    for (size_t i = 0; i < count; i++) {
        char* name = generate_array_dim_name(array_name, i);
        Symbol* s = SBfromVar(name, VT_NUM, false);
        ids[i] = s;

        STinsert(CURRENT_SCOPE, name, s);

        switch (orig) {
            case LOCAL_ORIGIN: s->offset = CURRENT_SCOPE->localvar_offset_counter++; break;
            case GLOBAL_ORIGIN: s->offset = GLOBAL_VAR_OFFSET++; break;
            default:
#ifdef DEBUGGING
                ERROR("Cannot get exprs on an imported variable");
#endif // DEBUGGING
        }

        TRAVexpr(exprs_node);
        if (LAST_TYPE != VT_NUM) {
            HAD_ERROR = true;
            USER_ERROR("Type mismatch; expected int but got %s", vt_to_str(LAST_TYPE));
        }

        exprs_node = EXPRS_NEXT(exprs_node);
        MEMfree(name);
    }

    return ids;
}

/**
 * Adds all types of parameters to function symbol. Prepends dimensions to arrays.
 * @param param_node starting param node
 * @param s function symbol
 * @param param_count amount of expected parameters
 */
static void find_param_types(node_st* param_node, const Symbol* s, const size_t param_count) {
    for (size_t i = 0; i < param_count; i++) {
        const size_t n_ids = count_ids(PARAM_DIMS(param_node));

        // Add dimensions before array
        for (size_t j = 0; j < n_ids; j++) {
            s->as.fun.param_types[i] = VT_NUM;
            i++;
        }

        s->as.fun.param_types[i] = ct_to_vt(PARAM_TYPE(param_node), n_ids > 0);

        param_node = PARAM_NEXT(param_node);
    }
}

/**
 * Finds all types of variables provided in a function call
 * @param exprs_node starting exprs node
 * @param count amount of arguments expected, including array dimensions
 * @return amount of arguments provided in function call
 */
static ValueType* find_funcall_types(node_st* exprs_node, const size_t count) {
    ValueType* types = MEMmalloc(sizeof(ValueType) * count);

    for (size_t i = 0; i < count; i++) {
#ifdef DEBUGGING
        ASSERT_MSG((exprs_node != NULL), "Exprs node was NULL on iteration %lu", i);
        ASSERT_MSG(EXPRS_EXPR(exprs_node), "Exprs_expr node was NULL on iteration %lu", i);
#endif // DEBUGGING

        if (NODE_TYPE(EXPRS_EXPR(exprs_node)) == NT_VAR) {
            node_st* var_node = EXPRS_EXPR(exprs_node);
#ifdef DEBUGGING
            ASSERT_MSG((var_node != NULL), "Exprs_expr was found to be a var, but node wasn't retrieved");
#endif // DEBUGGING
            Symbol* var_symbol = ScopeTreeFind(CURRENT_SCOPE, VAR_NAME(var_node));
            if (var_symbol == NULL) {
                HAD_ERROR = true;
                USER_ERROR("Cannot find variable named %s", VAR_NAME(var_node));
                return types;
            }

            if (IS_ARRAY(var_symbol->vtype)) {
                // Only add indices as parameters if array is not indexed
                if (VAR_INDICES(var_node) == NULL) {
                    // Add indices before array
                    for (size_t j = 0; j < var_symbol->as.array.dim_count; j++) {
                        types[i] = VT_NUM;
                        i++;
                    }

                    TRAVexpr(exprs_node);

                    // Add parameter type as array
                    types[i] = var_symbol->vtype;
                } else {
                    TRAVexpr(exprs_node);

                    // Array is demoted because it is indexed
                    types[i] = demote_array_type(var_symbol->vtype);
                }
            } else {
                TRAVexpr(exprs_node);
                types[i] = var_symbol->vtype;
            }

        } else {
            LAST_TYPE = VT_NULL;
            TRAVexpr(exprs_node);
            types[i] = LAST_TYPE;
        }

        exprs_node = EXPRS_NEXT(exprs_node);
    }

    return types;
}

/**
 * Generates a guaranteed unique name for any given nested function that
 * cannot be generated by a user function
 * @param s function header symbol to generate name for
 * @return unique name
 */
char* generate_unique_fun_label_name(const Symbol* s) {
    char* name = STRcpy(s->name);

    // Don't generate name for exported function
    if (s->exported) return name;

    while (s->parent_scope->parent_fun != NULL) {
        s = s->parent_scope->parent_fun;
        name = safe_concat_str(STRcpy(s->name), name);
    }

    return safe_concat_str(STRcpy("_"), name);
}

void CTAinit() {  }
void CTAfini() {  }

/**
 * @fn CTAprogram
 */
node_st *CTAprogram(node_st *node)
{
    CURRENT_SCOPE = STnew(NULL, NULL);
    GB_GLOBAL_SCOPE = CURRENT_SCOPE;

    /* First pass; we only look at function definitions, so we can correctly
     * handle usages of functions that have not been defined yet */
    PASS = DECLARATION_PASS;
    TRAVchildren(node);

    /* Second pass; we perform full analysis of the body now that we know
     * all definitions */
    PASS = ANALYSIS_PASS;
    TRAVchildren(node);

    // Exit here if an analysis error occurred
    exit_if_error();

    // If there are globals, we need an __init function in the bytecode
    GB_REQUIRES_INIT_FUNCTION = GLOBAL_VAR_OFFSET > 0;

    return node;
}

/**
 * @fn CTAdecls
 */
node_st *CTAdecls(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAexprs
 */
node_st *CTAexprs(node_st *node)
{
    // TODO: If we're being smart (like with counting IDs, we can probably get rid
    // of the SAVING_ARGS flag?

    // Track since array indexing requires integers as last type
    const bool was_indexing_array = INDEXING_ARRAY;

    INDEXING_ARRAY = false;
    TRAVexpr(node);
    INDEXING_ARRAY = was_indexing_array;

    if (INDEXING_ARRAY) {
        // Array indexes must always be integers
        if (LAST_TYPE != VT_NUM) {
            HAD_ERROR = true;
            USER_ERROR("Wrong index type in array index, expected int but got %s", vt_to_str(LAST_TYPE));
        }
    }

    TRAVnext(node);
    return node;
}

/**
 * @fn CTAarrexpr
 */
node_st *CTAarrexpr(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAids
 */
node_st *CTAids(node_st *node)
{
    // char* name = IDS_NAME(node);
    //
    // // Add name to scope as type integer
    // // TODO: Add to scope as symbol
    // // TODO: Add offset
    // STinsert(CURRENT_SCOPE, name, VT_NUM);

    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAexprstmt
 */
node_st *CTAexprstmt(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAreturn
 */
node_st *CTAreturn(node_st *node)
{
    TRAVchildren(node);

    // Find type of return expression
    const node_st* expr = RETURN_EXPR(node);
    const ValueType ret_type = expr == NULL ? VT_VOID : LAST_TYPE;

    // Check validity of return expression
    const ValueType parent_fun_type = CURRENT_SCOPE->parent_fun->vtype;
    if (ret_type != parent_fun_type) {
        HAD_ERROR = true;
        USER_ERROR("Trying to return %s from function with type %s",
            vt_to_str(ret_type), vt_to_str(parent_fun_type));
    }

    HAD_RETURN = true;

    return node;
}

/**
 * @fn CTAfuncall
 */
node_st *CTAfuncall(node_st *node)
{
    // Check if function exists and is actually a function
    char* name = FUNCALL_NAME(node);
    Symbol* s = ScopeTreeFind(CURRENT_SCOPE, name);
    if (!s) {
        HAD_ERROR = true;
        USER_ERROR("Function name %s does not exist in scope.", name);
        LAST_TYPE = VT_VOID;
        return node;
    }

    if (s->stype != ST_FUNCTION) {
        HAD_ERROR = true;
        USER_ERROR("Name %s is not defined as a function.", name);
        LAST_TYPE = VT_VOID;
        return node;
    }

    node_st* args_node = FUNCALL_FUN_ARGS(node);
    const size_t args_len = count_exprs(args_node);
    ValueType* types = find_funcall_types(args_node, args_len);

    // Arguments expected by function
    const ValueType* param_types = s->as.fun.param_types;
    const size_t params_len = s->as.fun.param_count;

    // Error if more arguments provided than parameters expected
    if (args_len > params_len) {
        HAD_ERROR = true;
        USER_ERROR("Too many arguments; got %lu but function %s only expects %lu",
            args_len, FUNCALL_NAME(node), params_len);
        MEMfree(types);
        return node;
    }

    // Error if more parameters expected than arguments provided
    if (args_len < params_len) {
        HAD_ERROR = true;
        USER_ERROR("Not enough arguments; got %lu but function %s expects %lu",
            args_len, FUNCALL_NAME(node), params_len);
        MEMfree(types);
        return node;
    }

    // Iterate through all params and arguments pairs and compare properties
    for (size_t i = 0; i < params_len; i++) {
        // Error if not similar type
        if (types[i] != param_types[i]) {
            HAD_ERROR = true;
            USER_ERROR("Argument type %s and parameter type %s don't match",
                vt_to_str(types[i]), vt_to_str(param_types[i]));
        }

        // TODO: Re-introduce this size check when I figure out how
        // if (IS_ARRAY(types[i])) {
        //     // Compare argument dimensions vs expected parameter dimensions
        //     if (args[i].arr_dim_count != param_dim_counts[i]) {
        //         HAD_ERROR = true;
        //         USER_ERROR("Argument has %lu dimensions, but function parameter expects %lu",
        //             args[i].arr_dim_count, param_dim_counts[i]);
        //     }
        // }
    }

    MEMfree(types);

    // Last type is function return type
    LAST_TYPE = s->vtype;
    return node;
}

/**
 * @fn CTAcast
 */
node_st *CTAcast(node_st *node)
{
    TRAVchildren(node);
    if (!(LAST_TYPE == VT_NUM || LAST_TYPE == VT_FLOAT || LAST_TYPE == VT_BOOL)) {
        HAD_ERROR = true;
        USER_ERROR("Invalid cast source, can only cast from Num, Float or Bool but got %s",
            vt_to_str(LAST_TYPE));
    }

    LAST_TYPE = ct_to_vt(CAST_TYPE(node), false);
    return node;
}

/**
 * @fn CTAfundefs
 */
node_st *CTAfundefs(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAfundef
 */
node_st *CTAfundef(node_st *node)
{
    char* fun_name = FUNDEF_NAME(node);
    const ValueType ret_type = ct_to_vt(FUNDEF_TYPE(node), false);

    if (PASS == DECLARATION_PASS) {
        /* First pass: only find information about self */
        const bool is_import = FUNDEF_IS_EXTERN(node);
        const bool is_export = FUNDEF_EXPORT(node);

        const size_t param_count = count_params(FUNDEF_PARAMS(node));
        Symbol* s = SBfromFun(fun_name, ret_type, param_count, is_import);

        // Find parameter types
        find_param_types(FUNDEF_PARAMS(node), s, s->as.fun.param_count);

        if (is_import) {
            // Add to import table
            s->imported = true;
            s->offset = FUN_IMPORT_OFFSET++;
        } else {
            // Create scope for function
            s->as.fun.scope = STnew(CURRENT_SCOPE, s);

            if (is_export) {
                // Add to export table
                s->exported = true;
                s->offset = FUN_EXPORT_OFFSET++;
            }
        }

        STinsert(CURRENT_SCOPE, fun_name, s);
    } else {
        /* Second pass: explore information about own statements
         * Here we also detect if variables are wrongly typed */

        Symbol* s = STlookup(CURRENT_SCOPE, fun_name);
        s->as.fun.label_name = generate_unique_fun_label_name(s);

        // Only explore if not extern
        if (!s->imported) {
            // Switch to new scope
            CURRENT_SCOPE = s->as.fun.scope;

            // Add parameters to scope
            TRAVparams(node);

            // Explore function body
            TRAVbody(node);

            // Reset loop counter
            CURRENT_SCOPE->for_loop_counter = 0;

            // Switch back to parent scope
            CURRENT_SCOPE = CURRENT_SCOPE->parent_scope;
        }
    }

    return node;
}

/**
 * @fn CTAfunbody
 */
node_st *CTAfunbody(node_st *node)
{
    // Explore inner functions and declarations that might be used by inner statements
    PASS = DECLARATION_PASS;
    TRAVdecls(node);
    TRAVlocal_fundefs(node);

    // Explore local functions and own statements now that we gathered all declarations
    PASS = ANALYSIS_PASS;
    TRAVlocal_fundefs(node);

    HAD_RETURN = false;
    TRAVstmts(node);
    if (CURRENT_SCOPE->parent_fun->vtype != VT_VOID && !HAD_RETURN) {
        HAD_ERROR = true;
        USER_ERROR("Missing return statement in function %s", CURRENT_SCOPE->parent_fun->name);
    }
    return node;
}

/**
 * @fn CTAifelse
 */
node_st *CTAifelse(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAwhile
 */
node_st *CTAwhile(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAdowhile
 */
node_st *CTAdowhile(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAfor
 */
node_st *CTAfor(node_st *node)
{
    char* name = FOR_VAR(node);
    char* adjusted_name = safe_concat_str(
        int_to_str((int) CURRENT_SCOPE->for_loop_counter),
        safe_concat_str(STRcpy("_"), STRcpy(name)));

    // Create for-loop entry in current scope
    Symbol* s_loop = SBfromForLoop(adjusted_name);
    STinsert(CURRENT_SCOPE, adjusted_name, s_loop);

    // Create for-loop scope and switch to it
    const size_t current_scope_depth = CURRENT_SCOPE->nesting_level;
    s_loop->as.forloop.scope = STnew(CURRENT_SCOPE, CURRENT_SCOPE->parent_fun);
    CURRENT_SCOPE = s_loop->as.forloop.scope;

    // Manually override nesting level
    CURRENT_SCOPE->nesting_level = current_scope_depth;

    // Create loop variable in new scope with correct offset
    Symbol* s_var = SBfromVar(name, VT_NUM, false);
    s_var->offset = CURRENT_SCOPE->parent_fun->as.fun.scope->localvar_offset_counter++;
    STinsert(CURRENT_SCOPE, name, s_var);

    // Create loop condition variable in scope with correct offset
    Symbol* s_cond = SBfromVar("_cond", VT_NUM, false);
    s_cond->offset = CURRENT_SCOPE->parent_fun->as.fun.scope->localvar_offset_counter++;
    STinsert(CURRENT_SCOPE, "_cond", s_cond);

    // Create loop step variable in scope with correct offset
    Symbol* s_step = SBfromVar("_step", VT_NUM, false);
    s_step->offset = CURRENT_SCOPE->parent_fun->as.fun.scope->localvar_offset_counter++;
    STinsert(CURRENT_SCOPE, "_step", s_step);

    // Check if all expressions are integers
    TRAVstart_expr(node);
    if (!LAST_TYPE == VT_NUM) {
        HAD_ERROR = true;
        USER_ERROR("Loop start condition expression must be an integer");
    }
    TRAVstop(node);
    if (!LAST_TYPE == VT_NUM) {
        HAD_ERROR = true;
        USER_ERROR("Loop start condition expression must be an integer");
    }
    TRAVstep(node);
    if (!LAST_TYPE == VT_NUM) {
        HAD_ERROR = true;
        USER_ERROR("Loop start condition expression must be an integer");
    }

    // Replace empty step for NUM(1)
    if (FOR_STEP(node) == NULL) {
        FOR_STEP(node) = ASTnum(1);
    }

    TRAVblock(node);

    // Special case: restore loop counter to zero for next traversal (during bytecode)
    CURRENT_SCOPE->for_loop_counter = 0;

    // Restore scope
    CURRENT_SCOPE = CURRENT_SCOPE->parent_scope;

    // Increment loop counter for next for-loop
    CURRENT_SCOPE->for_loop_counter++;

    // Clean up
    MEMfree(adjusted_name);

    return node;
}

/**
 * @fn CTAglobdecl
 */
node_st *CTAglobdecl(node_st *node)
{
    // Note: Requires array support

    // Immediate exit if phase is not DECLARATION_PASS to prevent adding twice
    if (PASS != DECLARATION_PASS) return node;

    char* name = GLOBDECL_NAME(node);

    HANDLE_DUPLICATE_ID(name);

    // Find dimensions, represented as IDs
    node_st* first_id = GLOBDECL_DIMS(node);
    const size_t n_dims = count_ids(first_id);

    const bool is_array = n_dims > 0;

    // Create symbols
    const ValueType type = ct_to_vt(GLOBDECL_TYPE(node), is_array);
    Symbol* s;
    if (is_array) {
        s = SBfromArray(name, type, true);

        // Create symbols for all index variables
        Symbol** ids = get_ids(first_id, n_dims, IMPORTED_ORIGIN);

        s->as.array.dim_count = n_dims;
        s->as.array.dims = ids;
    } else {
        s = SBfromVar(name, type, true);
    }

    s->offset = VAR_IMPORT_OFFSET++;
    STinsert(CURRENT_SCOPE, name, s);

    return node;
}

/**
 * @fn CTAglobdef
 */
node_st *CTAglobdef(node_st *node)
{
    // Note: Requires array support

    // Immediate exit if phase is not DECLARATION_PASS to prevent adding twice
    if (PASS != DECLARATION_PASS) return node;

    char* name = GLOBDEF_NAME(node);

    HANDLE_DUPLICATE_ID(name);

    // Find dimensions, represented as Exprs
    node_st* first_expr = GLOBDEF_DIMS(node);
    const size_t n_dims = count_exprs(first_expr);

    const bool is_array = n_dims > 0;

    // Create symbols
    const ValueType type = ct_to_vt(GLOBDEF_TYPE(node), is_array);
    Symbol* s;

    if (is_array) {
        s = SBfromArray(name, type, false);

        // Create symbols and add to table for all index variables
        // Also traverses the exprs so we don't need to manually
        Symbol** ids = get_exprs(first_expr, name, n_dims, GLOBAL_ORIGIN);

        s->as.array.dim_count = n_dims;
        s->as.array.dims = ids;

        if (GLOBDEF_INIT(node) != NULL && NODE_TYPE(GLOBDEF_INIT(node)) != NT_ARREXPR) {
            // Create hidden array variables for scalar initialisation
            char* scalar_symbol_name = safe_concat_str(STRcpy("_scalar_"), STRcpy(s->name));
            char* counter_symbol_name = safe_concat_str(STRcpy("_counter_"), STRcpy(s->name));
            char* size_symbol_name = safe_concat_str(STRcpy("_size_"), STRcpy(s->name));

            Symbol* scalar_symbol = SBfromVar(scalar_symbol_name, VT_NUM, false);
            Symbol* counter_symbol = SBfromVar(scalar_symbol_name, VT_NUM, false);
            Symbol* size_symbol = SBfromVar(scalar_symbol_name, VT_NUM, false);

            scalar_symbol->offset = GLOBAL_VAR_OFFSET++;
            counter_symbol->offset = GLOBAL_VAR_OFFSET++;
            size_symbol->offset = GLOBAL_VAR_OFFSET++;

            STinsert(CURRENT_SCOPE, scalar_symbol_name, scalar_symbol);
            STinsert(CURRENT_SCOPE, counter_symbol_name, counter_symbol);
            STinsert(CURRENT_SCOPE, size_symbol_name, size_symbol);

            MEMfree(scalar_symbol_name);
            MEMfree(counter_symbol_name);
            MEMfree(size_symbol_name);
        }
    } else {
        s = SBfromVar(name, type, false);
    }

    TRAVinit(node);

    s->offset = GLOBAL_VAR_OFFSET++;
    STinsert(CURRENT_SCOPE, name, s);

    // Typecheck init; demote array
    if (GLOBDEF_INIT(node) != NULL && ct_to_vt(GLOBDEF_TYPE(node), false) != LAST_TYPE) {
        HAD_ERROR = true;
        USER_ERROR("Variable of type %s was initialised with expression of type %s",
            vt_to_str(ct_to_vt(GLOBDEF_TYPE(node), is_array)),
            vt_to_str(LAST_TYPE));
    }

    // TODO: Allow scalar to init array

    return node;
}

/**
 * @fn CTAparam
 */
node_st *CTAparam(node_st *node)
{
    // Note: Requires array support

    char* name = PARAM_NAME(node);

    HANDLE_DUPLICATE_ID(name);

    // Find dimensions, represented as IDs
    node_st* first_id = PARAM_DIMS(node);
    const size_t n_dims = count_ids(first_id);

    const bool is_array = n_dims > 0;

    // Create symbols
    const ValueType type = ct_to_vt(PARAM_TYPE(node), is_array);
    Symbol* s;
    if (is_array) {
        s = SBfromArray(name, type, false);

        // Create symbols for all index variables
        Symbol** ids = get_ids(first_id, n_dims, LOCAL_ORIGIN);

        // Once again check for duplicate ID, since array size ID might have same name as array
        HANDLE_DUPLICATE_ID(name);

        s->as.array.dim_count = n_dims;
        s->as.array.dims = ids;
    } else {
        s = SBfromVar(name, type, false);
    }

    s->offset = CURRENT_SCOPE->localvar_offset_counter++;
    STinsert(CURRENT_SCOPE, name, s);

    TRAVnext(node);
    return node;
}

/**
 * @fn CTAvardecl
 */
node_st *CTAvardecl(node_st *node)
{
    // Note: Requires array support

    char* name = VARDECL_NAME(node);

    HANDLE_DUPLICATE_ID(name);

    // Find dimensions, represented as Exprs
    node_st* first_expr = VARDECL_DIMS(node);
    const size_t n_dims = count_exprs(first_expr);

    const bool is_array = n_dims > 0;

    // Create symbols
    const ValueType type = ct_to_vt(VARDECL_TYPE(node), is_array);
    Symbol* s;

    if (is_array) {
        s = SBfromArray(name, type, false);

        // Create symbols and add to table for all index variables
        // Also traverses the exprs so we don't need to manually
        Symbol** ids = get_exprs(first_expr, name, n_dims, LOCAL_ORIGIN);

        s->as.array.dim_count = n_dims;
        s->as.array.dims = ids;

        if (VARDECL_INIT(node) != NULL && NODE_TYPE(VARDECL_INIT(node)) != NT_ARREXPR) {
            // Create hidden array variables for scalar initialisation
            char* scalar_symbol_name = safe_concat_str(STRcpy("_scalar_"), STRcpy(s->name));
            char* counter_symbol_name = safe_concat_str(STRcpy("_counter_"), STRcpy(s->name));
            char* size_symbol_name = safe_concat_str(STRcpy("_size_"), STRcpy(s->name));

            Symbol* scalar_symbol = SBfromVar(scalar_symbol_name, VT_NUM, false);
            Symbol* counter_symbol = SBfromVar(scalar_symbol_name, VT_NUM, false);
            Symbol* size_symbol = SBfromVar(scalar_symbol_name, VT_NUM, false);

            scalar_symbol->offset = CURRENT_SCOPE->localvar_offset_counter++;
            counter_symbol->offset = CURRENT_SCOPE->localvar_offset_counter++;
            size_symbol->offset = CURRENT_SCOPE->localvar_offset_counter++;

            STinsert(CURRENT_SCOPE, scalar_symbol_name, scalar_symbol);
            STinsert(CURRENT_SCOPE, counter_symbol_name, counter_symbol);
            STinsert(CURRENT_SCOPE, size_symbol_name, size_symbol);

            MEMfree(scalar_symbol_name);
            MEMfree(counter_symbol_name);
            MEMfree(size_symbol_name);
        }
    } else {
        s = SBfromVar(name, type, false);
    }

    // Traverse init before adding to scope to allow for correct shadowing and use-before-initialisation
    TRAVinit(node);

    s->offset = CURRENT_SCOPE->localvar_offset_counter++;
    STinsert(CURRENT_SCOPE, name, s);

    // Typecheck init, demote array
    if (VARDECL_INIT(node) != NULL && ct_to_vt(VARDECL_TYPE(node), false) != LAST_TYPE) {
        HAD_ERROR = true;
        USER_ERROR("Variable of type %s was initialised with expression of type %s",
            vt_to_str(ct_to_vt(VARDECL_TYPE(node), is_array)),
            vt_to_str(LAST_TYPE));
    }

    // TODO: Allow scalar to init array

    TRAVnext(node);
    return node;
}

/**
 * @fn CTAstmts
 */
node_st *CTAstmts(node_st *node)
{
    TRAVchildren(node);
    return node;
}

/**
 * @fn CTAassign
 */
node_st *CTAassign(node_st *node)
{
    // Note: Might require array support?

    TRAVlet(node);
    const ValueType let_type = LAST_TYPE;
    const bool let_is_arith = IS_ARITH_TYPE(let_type);

    TRAVchildren(node);
    const ValueType expr_type = LAST_TYPE;
    const bool expr_is_arith = IS_ARITH_TYPE(expr_type);

    // Numbers are compatible
    if (let_is_arith && expr_is_arith) {
        // If types are both arith but not same, implicit conversion takes place
        if (let_type == VT_NUM && expr_type == VT_FLOAT) {
            ASSIGN_EXPR(node) = ASTcast(ASSIGN_EXPR(node), CT_int);
        } else if (let_type == VT_FLOAT && expr_type == VT_NUM) {
            ASSIGN_EXPR(node) = ASTcast(ASSIGN_EXPR(node), CT_float);
        }
        LAST_TYPE = let_type == VT_FLOAT || expr_type == VT_FLOAT ? VT_FLOAT : VT_NUM;
    }

    // Booleans are compatible
    else if (let_type == VT_BOOL && expr_type == VT_BOOL) {
        LAST_TYPE = VT_BOOL;
    }

    else {
        HAD_ERROR = true;
        USER_ERROR("Operands not compatible: %s and %s",
            vt_to_str(let_type), vt_to_str(expr_type));
    }

    return node;
}

/**
 * @fn CTAbinop
 */
node_st *CTAbinop(node_st *node)
{
    TRAVleft(node);
    const ValueType left_type = LAST_TYPE;
    const bool left_is_arith = IS_ARITH_TYPE(left_type);

    TRAVright(node);
    const ValueType right_type = LAST_TYPE;
    const bool right_is_arith = IS_ARITH_TYPE(right_type);

    // Numbers are compatible
    if (left_is_arith && right_is_arith) {
        // If either argument is float, int is implicitly cast otherwise num
        if (left_type == VT_NUM && right_type == VT_FLOAT) {
            BINOP_LEFT(node) = ASTcast(BINOP_LEFT(node), CT_float);
        } else if (left_type == VT_FLOAT && right_type == VT_NUM) {
            BINOP_RIGHT(node) = ASTcast(BINOP_RIGHT(node), CT_float);
        }

        LAST_TYPE = left_type == VT_FLOAT || right_type == VT_FLOAT ? VT_FLOAT : VT_NUM;
    }

    // Booleans are compatible
    else if (left_type == VT_BOOL && right_type == VT_BOOL) {
        LAST_TYPE = VT_BOOL;
    }

    else {
        HAD_ERROR = true;
        USER_ERROR("Operands not compatible: %s and %s",
            vt_to_str(left_type), vt_to_str(right_type));
    }

    // Check individual operators with types
    const enum BinOpType t = BINOP_OP(node);
    if (left_is_arith) {
        if (!(t == BO_add || t == BO_sub || t == BO_mul || t == BO_div || t == BO_mod ||
              t == BO_eq || t == BO_ne || t == BO_lt || t == BO_le || t == BO_gt || t == BO_ge)) {
            HAD_ERROR = true;
            USER_ERROR("Invalid operands %s and %s for operator %s",
                vt_to_str(left_type), vt_to_str(right_type), bo_to_str(t));
        }

        // If equality operator, LAST_TYPE should become boolean
        if (t == BO_eq || t == BO_ne || t == BO_lt || t == BO_le || t == BO_gt || t == BO_ge) {
            LAST_TYPE = VT_BOOL;
        }
    } else if (left_type == VT_BOOL) {
        // BO_add and BO_mul are strict logic disjunction and conjunction, respectively
        if (!(t == BO_add || t == BO_mul || t == BO_eq || t == BO_ne || t == BO_and || t == BO_or)) {
            HAD_ERROR = true;
            USER_ERROR("Invalid operands %s and %s for operator %s",
                vt_to_str(left_type), vt_to_str(right_type), bo_to_str(t));
        }
        LAST_TYPE = VT_BOOL;
    } else {
#ifdef DEBUGGING
        ERROR("Unexpected operand %s is not arith or bool", vt_to_str(left_type));
#endif // DEBUGGING
    }

    return node;
}

/**
 * @fn CTAmonop
 */
node_st *CTAmonop(node_st *node)
{
    TRAVoperand(node);

    // Confirm type is correct
    switch (MONOP_OP(node)) {
        case MO_neg:
            if (!(LAST_TYPE == VT_NUM || LAST_TYPE == VT_FLOAT)) {
                HAD_ERROR = true;
                USER_ERROR("Expected operand type Int or Float, got %s instead", vt_to_str(LAST_TYPE));
            }
            break;
        case MO_not: if (LAST_TYPE != VT_BOOL) {
                HAD_ERROR = true;
                USER_ERROR("Expected operand type Bool, got %s instead", vt_to_str(LAST_TYPE));
            }
            break;
        default: /* Should never occur */
            HAD_ERROR = true;
            USER_ERROR("Unknown monop %s with operand %s", mo_to_str(MONOP_OP(node)), vt_to_str(LAST_TYPE));
    }

    return node;
}

/**
 * @fn CTAvarlet
 */
node_st *CTAvarlet(node_st *node)
{
    // Note: Requires array support

    TRAVchildren(node);

    char* name = VARLET_NAME(node);

    // Look up variable
    const Symbol* s = ScopeTreeFind(CURRENT_SCOPE, name);

    // Handle case of missing symbol
    HANDLE_MISSING_SYMBOL(name, s);

    // Find dimensions, represented as Exprs
    node_st* first_expr = VARLET_INDICES(node);
    const size_t n_dims = count_exprs(first_expr);

    const bool is_array = n_dims > 0;

    if (is_array) {
        // Typecheck is symbol is indeed declared as an array
        if (!IS_ARRAY(s->vtype)) {
            HAD_ERROR = true;
            USER_ERROR("Identifier %s was indexed, but is not an array", name);
        }

        // Ensure dimensions match
        if (s->as.array.dim_count != n_dims) {
            HAD_ERROR = true;
            USER_ERROR("Expected array dimension count is %lu but got %lu", s->as.array.dim_count, n_dims);
        }

        // Demote array since it's indexed and stores a single value
        LAST_TYPE = demote_array_type(s->vtype);
    } else {
        LAST_TYPE = s->vtype;
    }

    return node;
}

/**
 * @fn CTAvar
 */
node_st *CTAvar(node_st *node)
{
    // Note: Requires array support

    TRAVchildren(node);

    char* name = VAR_NAME(node);

    // Look up variable
    Symbol* s = ScopeTreeFind(CURRENT_SCOPE, name);

    // Handle case of missing symbol
    HANDLE_MISSING_SYMBOL(name, s);

    // Find dimensions, represented as Exprs
    node_st* first_expr = VAR_INDICES(node);
    const size_t n_dims = count_exprs(first_expr);

    const bool is_array = n_dims > 0;

    if (is_array) {
        // Typecheck is symbol is indeed declared as an array
        if (!IS_ARRAY(s->vtype)) {
            HAD_ERROR = true;
            USER_ERROR("Identifier %s was indexed, but is not an array", name);
        }

        // Ensure dimensions match
        if (s->as.array.dim_count != n_dims) {
            HAD_ERROR = true;
            USER_ERROR("Expected array dimension count is %lu but got %lu", s->as.array.dim_count, n_dims);
        }

        // Demote array since it's indexed and retrieves a single value
        LAST_TYPE = demote_array_type(s->vtype);
    } else {
        LAST_TYPE = s->vtype;
    }

    // Add symbol to AST to retrieve in bytecode
    // Ensures correct variable shadowing and use-before-declaration
    VAR_SYMBOL(node) = s;

    return node;
}

/**
 * @fn CTAnum
 */
node_st *CTAnum(node_st *node)
{
    TRAVchildren(node);
    LAST_TYPE = VT_NUM;
    return node;
}

/**
 * @fn CTAfloat
 */
node_st *CTAfloat(node_st *node)
{
    TRAVchildren(node);
    LAST_TYPE = VT_FLOAT;
    return node;
}

/**
 * @fn CTAbool
 */
node_st *CTAbool(node_st *node)
{
    TRAVchildren(node);
    LAST_TYPE = VT_BOOL;
    return node;
}

