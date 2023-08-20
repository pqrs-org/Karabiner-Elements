/*
 *  Shared string macros.
 *
 *  Using shared macros helps minimize strings data size because it's easy
 *  to check if an existing string could be used.  String constants don't
 *  need to be all defined here; defining a string here makes sense if there's
 *  a high chance the string could be reused.  Also, using macros allows
 *  a call site express the exact string needed, but the macro may map to an
 *  approximate string to reduce unique string count.  Macros can also be
 *  more easily tuned for low memory targets than #if defined()s throughout
 *  the code base.
 *
 *  Because format strings behave differently in the call site (they need to
 *  be followed by format arguments), they use a special prefix DUK_STR_FMT_.
 *
 *  On some compilers using explicit shared strings is preferable; on others
 *  it may be better to use straight literals because the compiler will combine
 *  them anyway, and such strings won't end up unnecessarily in a symbol table.
 */

#if !defined(DUK_ERRMSG_H_INCLUDED)
#define DUK_ERRMSG_H_INCLUDED

/* Mostly API and built-in method related */
#define DUK_STR_INTERNAL_ERROR                  "internal error"
#define DUK_STR_UNSUPPORTED                     "unsupported"
#define DUK_STR_INVALID_COUNT                   "invalid count"
#define DUK_STR_INVALID_ARGS                    "invalid args"
#define DUK_STR_INVALID_STATE                   "invalid state"
#define DUK_STR_INVALID_INPUT                   "invalid input"
#define DUK_STR_INVALID_LENGTH                  "invalid length"
#define DUK_STR_NOT_CONSTRUCTABLE               "not constructable"
#define DUK_STR_CONSTRUCT_ONLY                  "constructor requires 'new'"
#define DUK_STR_NOT_CALLABLE                    "not callable"
#define DUK_STR_NOT_EXTENSIBLE                  "not extensible"
#define DUK_STR_NOT_WRITABLE                    "not writable"
#define DUK_STR_NOT_CONFIGURABLE                "not configurable"
#define DUK_STR_INVALID_CONTEXT                 "invalid context"
#define DUK_STR_INVALID_INDEX                   "invalid args"
#define DUK_STR_PUSH_BEYOND_ALLOC_STACK         "cannot push beyond allocated stack"
#define DUK_STR_NOT_UNDEFINED                   "unexpected type"
#define DUK_STR_NOT_NULL                        "unexpected type"
#define DUK_STR_NOT_BOOLEAN                     "unexpected type"
#define DUK_STR_NOT_NUMBER                      "unexpected type"
#define DUK_STR_NOT_STRING                      "unexpected type"
#define DUK_STR_NOT_OBJECT                      "unexpected type"
#define DUK_STR_NOT_POINTER                     "unexpected type"
#define DUK_STR_NOT_BUFFER                      "not buffer" /* still in use with verbose messages */
#define DUK_STR_UNEXPECTED_TYPE                 "unexpected type"
#define DUK_STR_NOT_THREAD                      "unexpected type"
#define DUK_STR_NOT_COMPFUNC                    "unexpected type"
#define DUK_STR_NOT_NATFUNC                     "unexpected type"
#define DUK_STR_NOT_C_FUNCTION                  "unexpected type"
#define DUK_STR_NOT_FUNCTION                    "unexpected type"
#define DUK_STR_NOT_REGEXP                      "unexpected type"
#define DUK_STR_TOPRIMITIVE_FAILED              "coercion to primitive failed"
#define DUK_STR_NUMBER_OUTSIDE_RANGE            "number outside range"
#define DUK_STR_NOT_OBJECT_COERCIBLE            "not object coercible"
#define DUK_STR_CANNOT_NUMBER_COERCE_SYMBOL     "cannot number coerce Symbol"
#define DUK_STR_CANNOT_STRING_COERCE_SYMBOL     "cannot string coerce Symbol"
#define DUK_STR_STRING_TOO_LONG                 "string too long"
#define DUK_STR_BUFFER_TOO_LONG                 "buffer too long"
#define DUK_STR_ALLOC_FAILED                    "alloc failed"
#define DUK_STR_WRONG_BUFFER_TYPE               "wrong buffer type"
#define DUK_STR_BASE64_ENCODE_FAILED            "base64 encode failed"
#define DUK_STR_SOURCE_DECODE_FAILED            "source decode failed"
#define DUK_STR_UTF8_DECODE_FAILED              "utf-8 decode failed"
#define DUK_STR_BASE64_DECODE_FAILED            "base64 decode failed"
#define DUK_STR_HEX_DECODE_FAILED               "hex decode failed"
#define DUK_STR_INVALID_BYTECODE                "invalid bytecode"
#define DUK_STR_NO_SOURCECODE                   "no sourcecode"
#define DUK_STR_RESULT_TOO_LONG                 "result too long"
#define DUK_STR_INVALID_CFUNC_RC                "invalid C function rc"
#define DUK_STR_INVALID_INSTANCEOF_RVAL         "invalid instanceof rval"
#define DUK_STR_INVALID_INSTANCEOF_RVAL_NOPROTO "instanceof rval has no .prototype"

/* JSON */
#define DUK_STR_FMT_PTR          "%p"
#define DUK_STR_FMT_INVALID_JSON "invalid json (at offset %ld)"
#define DUK_STR_CYCLIC_INPUT     "cyclic input"

/* Generic codec */
#define DUK_STR_DEC_RECLIMIT "decode recursion limit"
#define DUK_STR_ENC_RECLIMIT "encode recursion limit"

/* Object property access */
#define DUK_STR_INVALID_BASE         "invalid base value"
#define DUK_STR_STRICT_CALLER_READ   "cannot read strict 'caller'"
#define DUK_STR_PROXY_REJECTED       "proxy rejected"
#define DUK_STR_INVALID_ARRAY_LENGTH "invalid array length"
#define DUK_STR_SETTER_UNDEFINED     "setter undefined"
#define DUK_STR_INVALID_DESCRIPTOR   "invalid descriptor"

/* Proxy */
#define DUK_STR_PROXY_REVOKED       "proxy revoked"
#define DUK_STR_INVALID_TRAP_RESULT "invalid trap result"

/* Variables */

/* Lexer */
#define DUK_STR_INVALID_ESCAPE          "invalid escape"
#define DUK_STR_UNTERMINATED_STRING     "unterminated string"
#define DUK_STR_UNTERMINATED_COMMENT    "unterminated comment"
#define DUK_STR_UNTERMINATED_REGEXP     "unterminated regexp"
#define DUK_STR_TOKEN_LIMIT             "token limit"
#define DUK_STR_REGEXP_SUPPORT_DISABLED "regexp support disabled"
#define DUK_STR_INVALID_NUMBER_LITERAL  "invalid number literal"
#define DUK_STR_INVALID_TOKEN           "invalid token"

/* Compiler */
#define DUK_STR_PARSE_ERROR              "parse error"
#define DUK_STR_DUPLICATE_LABEL          "duplicate label"
#define DUK_STR_INVALID_LABEL            "invalid label"
#define DUK_STR_INVALID_ARRAY_LITERAL    "invalid array literal"
#define DUK_STR_INVALID_OBJECT_LITERAL   "invalid object literal"
#define DUK_STR_INVALID_VAR_DECLARATION  "invalid variable declaration"
#define DUK_STR_CANNOT_DELETE_IDENTIFIER "cannot delete identifier"
#define DUK_STR_INVALID_EXPRESSION       "invalid expression"
#define DUK_STR_INVALID_LVALUE           "invalid lvalue"
#define DUK_STR_INVALID_NEWTARGET        "invalid new.target"
#define DUK_STR_EXPECTED_IDENTIFIER      "expected identifier"
#define DUK_STR_EMPTY_EXPR_NOT_ALLOWED   "empty expression not allowed"
#define DUK_STR_INVALID_FOR              "invalid for statement"
#define DUK_STR_INVALID_SWITCH           "invalid switch statement"
#define DUK_STR_INVALID_BREAK_CONT_LABEL "invalid break/continue label"
#define DUK_STR_INVALID_RETURN           "invalid return"
#define DUK_STR_INVALID_TRY              "invalid try"
#define DUK_STR_INVALID_THROW            "invalid throw"
#define DUK_STR_WITH_IN_STRICT_MODE      "with in strict mode"
#define DUK_STR_FUNC_STMT_NOT_ALLOWED    "function statement not allowed"
#define DUK_STR_UNTERMINATED_STMT        "unterminated statement"
#define DUK_STR_INVALID_ARG_NAME         "invalid argument name"
#define DUK_STR_INVALID_FUNC_NAME        "invalid function name"
#define DUK_STR_INVALID_GETSET_NAME      "invalid getter/setter name"
#define DUK_STR_FUNC_NAME_REQUIRED       "function name required"

/* RegExp */
#define DUK_STR_INVALID_QUANTIFIER         "invalid regexp quantifier"
#define DUK_STR_INVALID_QUANTIFIER_NO_ATOM "quantifier without preceding atom"
#define DUK_STR_INVALID_QUANTIFIER_VALUES  "quantifier values invalid (qmin > qmax)"
#define DUK_STR_QUANTIFIER_TOO_MANY_COPIES "quantifier requires too many atom copies"
#define DUK_STR_UNEXPECTED_CLOSING_PAREN   "unexpected closing parenthesis"
#define DUK_STR_UNEXPECTED_END_OF_PATTERN  "unexpected end of pattern"
#define DUK_STR_UNEXPECTED_REGEXP_TOKEN    "unexpected token in regexp"
#define DUK_STR_INVALID_REGEXP_FLAGS       "invalid regexp flags"
#define DUK_STR_INVALID_REGEXP_ESCAPE      "invalid regexp escape"
#define DUK_STR_INVALID_BACKREFS           "invalid backreference(s)"
#define DUK_STR_INVALID_REGEXP_CHARACTER   "invalid regexp character"
#define DUK_STR_INVALID_REGEXP_GROUP       "invalid regexp group"
#define DUK_STR_UNTERMINATED_CHARCLASS     "unterminated character class"
#define DUK_STR_INVALID_RANGE              "invalid range"

/* Limits */
#define DUK_STR_VALSTACK_LIMIT                  "valstack limit"
#define DUK_STR_CALLSTACK_LIMIT                 "callstack limit"
#define DUK_STR_PROTOTYPE_CHAIN_LIMIT           "prototype chain limit"
#define DUK_STR_BOUND_CHAIN_LIMIT               "function call bound chain limit"
#define DUK_STR_NATIVE_STACK_LIMIT              "C stack depth limit"
#define DUK_STR_COMPILER_RECURSION_LIMIT        "compiler recursion limit"
#define DUK_STR_BYTECODE_LIMIT                  "bytecode limit"
#define DUK_STR_REG_LIMIT                       "register limit"
#define DUK_STR_TEMP_LIMIT                      "temp limit"
#define DUK_STR_CONST_LIMIT                     "const limit"
#define DUK_STR_FUNC_LIMIT                      "function limit"
#define DUK_STR_REGEXP_COMPILER_RECURSION_LIMIT "regexp compiler recursion limit"
#define DUK_STR_REGEXP_EXECUTOR_RECURSION_LIMIT "regexp executor recursion limit"
#define DUK_STR_REGEXP_EXECUTOR_STEP_LIMIT      "regexp step limit"

#endif /* DUK_ERRMSG_H_INCLUDED */
