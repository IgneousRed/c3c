// Copyright (c) 2019 Christoffer Lerno. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "compiler_internal.h"

typedef bool(*ExprAnalysis)(Context *, Expr*);
typedef bool(*ExprBinopAnalysis)(Context *, Expr*, Expr*, Expr*);
typedef bool(*ExprUnaryAnalysis)(Context *, Expr*, Expr*);

static ExprBinopAnalysis BINOP_ANALYSIS[TOKEN_EOF];
static ExprUnaryAnalysis UNARYOP_ANALYSIS[TOKEN_EOF + 1];
static ExprUnaryAnalysis POSTUNARYOP_ANALYSIS[TOKEN_EOF + 1];

static inline bool sema_type_error_on_binop(const char *op, Expr *expr)
{
	SEMA_ERROR(expr->loc, "Cannot perform '%s' %s '%s'.", expr->binary_expr.left->type->name_loc.string, op, expr->binary_expr.right->type->name_loc.string);
	return false;
}


static inline bool sema_expr_analyse_conditional(Context *context, Expr *expr)
{
	TODO
}

static inline bool sema_expr_analyse_identifier(Context *context, Expr *expr)
{
	// TODO what about struct functions
	if (expr->identifier_expr.module.string)
	{
		TODO
	}
	Decl *decl = context_find_ident(context, expr->identifier_expr.identifier.string);
	if (decl == NULL)
	{
		SEMA_ERROR(expr->loc, "Unknown identifier %s.", expr->identifier_expr.identifier.string);
		return false;
	}
	expr->identifier_expr.decl = decl;
	expr->type = decl->var.type;
	return true;
}

static inline bool sema_expr_analyse_var_call(Context *context, Expr *expr) { TODO }
static inline bool sema_expr_analyse_macro_call(Context *context, Expr *expr, Decl *macro)
{
	Ast macro_parent;
	// TODO handle loops
	Decl *stored_macro = context->evaluating_macro;
	Type *stored_rtype = context->rtype;
	context->evaluating_macro = macro;
	context->rtype = macro->macro_decl.rtype;
	// Handle escaping macro
	bool success = sema_analyse_statement(context, macro->macro_decl.body);
	context->evaluating_macro = stored_macro;
	context->rtype = stored_rtype;
	if (!success) return false;

	TODO
	return success;
};
static inline bool sema_expr_analyse_generic_call(Context *context, Expr *expr) { TODO };

static inline bool sema_expr_analyse_func_call(Context *context, Expr *expr, Decl *decl)
{
	if (decl->func.function_signature.throws != NULL) TODO
	Expr **args =expr->call_expr.arguments;
	Decl **func_params = decl->func.function_signature.params;
	unsigned num_args = vec_size(args);
	// unsigned num_params = vec_size(func_params);
	// TODO handle named parameters, handle default parameters, varargs etc
	for (unsigned i = 0; i < num_args; i++)
	{
		Expr *arg = args[i];
		if (!sema_expr_analysis(context, arg)) return false;
		if (!cast(arg, func_params[i]->var.type, CAST_TYPE_IMPLICIT_ASSIGN)) return false;
	}
	expr->type = decl->func.function_signature.rtype;
	return true;
}

static inline bool sema_expr_analyse_call(Context *context, Expr *expr)
{
	Expr *func_expr = expr->call_expr.function;
	if (!sema_expr_analysis(context, func_expr)) return false;
	if (func_expr->expr_kind != EXPR_IDENTIFIER)
	{
		TODO
	}
	Decl *decl = func_expr->identifier_expr.decl;
	switch (decl->decl_kind)
	{
		case DECL_VAR:
			return sema_expr_analyse_var_call(context, expr);
		case DECL_FUNC:
			return sema_expr_analyse_func_call(context, expr, decl);
		case DECL_MACRO:
			return sema_expr_analyse_macro_call(context, expr, decl);
		case DECL_GENERIC:
			return sema_expr_analyse_generic_call(context, expr);
		default:
			SEMA_ERROR(expr->loc, "The expression cannot be called.");
			return false;
			break;
	}
}

static inline bool sema_expr_analyse_struct_value(Context *context, Expr *expr)
{
	TODO
}

static inline bool sema_expr_analyse_struct_init_values(Context *context, Expr *expr)
{
	TODO
}

static inline bool sema_expr_analyse_subscript(Context *context, Expr *expr)
{
	TODO
}

static inline bool sema_expr_analyse_access(Context *context, Expr *expr)
{
	TODO
}

static inline bool sema_expr_analyse_method_ref(Context *context, Expr *expr)
{
	TODO
}


static inline bool sema_expr_analyse_initializer_list(Context *context, Expr *expr)
{
	TODO
}

static inline bool sema_expr_analyse_sizeof(Context *context, Expr *expr)
{
	TODO
}

static inline bool sema_expr_analyse_cast(Context *context, Expr *expr)
{
	Expr *inner = expr->expr_cast.expr;
	if (!sema_resolve_type(context, expr->type)) return false;
	if (!sema_expr_analysis(context, inner)) return false;

	if (!cast(inner, expr->type, CAST_TYPE_EXPLICIT)) return false;

	// Overwrite cast.
	Token loc = expr->loc;
	*expr = *inner;
	expr->loc = loc;

	return true;
}


static bool sema_expr_analyse_assign(Context *context, Expr *expr, Expr *left, Expr *right)
{
	if (!cast(right, left->type, CAST_TYPE_IMPLICIT_ASSIGN)) return false;
	// Check assignable
	return true;
}

static inline bool both_const(Expr *left, Expr *right)
{
	return left->expr_kind == EXPR_CONST && right->expr_kind == EXPR_CONST;
}

static bool sema_expr_analyse_add(Context *context, Expr *expr, Expr *left, Expr *right)
{
	Type *left_type = left->type->canonical;
	Type *right_type = right->type->canonical;
	if (left_type->type_kind == TYPE_POINTER)
	{
		if (!cast_arithmetic(right, left, "+")) return false;
		expr->type = left_type;
		return true;
	}
	if (right_type->type_kind == TYPE_POINTER)
	{
		if (!cast_arithmetic(left, right, "+")) return false;
		expr->type = right_type;
		return true;
	}
	if (!cast_arithmetic(left, right, "+")) return false;
	if (!cast_arithmetic(right, left, "+")) return false;

	Type *canonical = left->type->canonical;
	if (!type_is_number(canonical))
	{
		SEMA_ERROR(expr->loc, "Add is not allowed");
		return false;
	}
	if (both_const(left, right))
	{
		switch (left->const_expr.type)
		{
			case CONST_INT:
				expr->const_expr.i = left->const_expr.i + right->const_expr.i;
				break;
			case CONST_FLOAT:
				expr->const_expr.f = left->const_expr.f + right->const_expr.f;
				break;
			default:
				UNREACHABLE
		}
		expr->expr_kind = EXPR_CONST;
		expr->const_expr.type = left->const_expr.type;
	}
	expr->type = left->type;
	return true;
}
static bool sema_expr_analyse_add_assign(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_sub(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_sub_assign(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }

static bool sema_expr_analyse_mult(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_mult_assign(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_div(Context *context, Expr *expr, Expr *left, Expr *right)
{
	/*
	switch (left->type)
	{
		case CONST_NIL:
		case CONST_BOOL:
		case CONST_STRING:
			UNREACHABLE;
		case CONST_INT:
			if (right->i == 0)
			{
				SEMA_ERROR(expr->binary_expr.right->loc, "Division by zero not allowed.");
				return false;
			}
			result->i = left->i / right->i;
			break;
		case CONST_FLOAT:
			if (right->f == 0)
			{
				SEMA_ERROR(expr->binary_expr.right->loc, "Division by zero not allowed.");
				return false;
			}
			expr->const_expr.f = left->f / right->f;
			expr->const_expr.type = CONST_FLOAT;
			break;
	}*/
	return false;
}

static bool sema_expr_analyse_div_assign(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_mod(Context *context, Expr *expr, Expr *left, Expr *right)
{
	if (!cast_arithmetic(left, right, "%")) return false;
	if (!type_is_integer(right->type->canonical) || !type_is_integer(left->type->canonical)) return sema_type_error_on_binop("%", expr);

	if (right->expr_kind == EXPR_CONST)
	{
		if (right->const_expr.i == 0)
		{
			SEMA_ERROR(expr->binary_expr.right->loc, "Cannot perform mod by zero.");
			return false;
		}
		if (left->expr_kind == EXPR_CONST)
		{
			// TODO negative
			expr->const_expr.i = left->const_expr.i / right->const_expr.i;
			expr->type = right->type;
			expr->expr_kind = EXPR_CONST;
			expr->const_expr.type = CONST_INT;
		}
	}
	return true;
}

static bool sema_expr_analyse_mod_assign(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }

static bool sema_expr_analyse_bit_and(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_bit_and_assign(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_bit_or(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_bit_or_assign(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_bit_xor(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_bit_xor_assign(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_shr(Context *context, Expr *expr, Expr *left, Expr *right)
{ TODO }
static bool sema_expr_analyse_shr_assign(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_shl(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_shl_assign(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }

static bool sema_expr_analyse_and(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_and_assign(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_or(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_or_assign(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }

static bool sema_expr_analyse_eq(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_ne(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_ge(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_gt(Context *context, Expr *expr, Expr *left, Expr *right)
{
	if (!cast_arithmetic(left, right, ">")) return false;
	if (!cast_arithmetic(right, left, ">")) return false;
	if (both_const(left, right))
	{
		switch (left->const_expr.type)
		{
			case CONST_FLOAT:
				expr->const_expr.b = left->const_expr.f > right->const_expr.f;
				break;
			case CONST_BOOL:
				expr->const_expr.b = left->const_expr.b > right->const_expr.b;
				break;
			case CONST_INT:
				expr->const_expr.b = left->const_expr.i > right->const_expr.i;
				break;
			default:
				UNREACHABLE
		}
		expr->const_expr.type = CONST_BOOL;
		expr->expr_kind = EXPR_CONST;
	}
	if (!cast_to_runtime(left) || !cast_to_runtime(right)) return false;
	expr->type = type_bool;
	return true;
}
static bool sema_expr_analyse_le(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_lt(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }
static bool sema_expr_analyse_elvis(Context *context, Expr *expr, Expr *left, Expr *right) { TODO }


static bool sema_expr_analyse_deref(Context *context, Expr *expr, Expr *inner) { TODO }
static bool sema_expr_analyse_addr(Context *context, Expr *expr, Expr *inner) { TODO }

static bool sema_expr_analyse_neg(Context *context, Expr *expr, Expr *inner)
{
	Type *canonical = inner->type->canonical;
	if (!builtin_may_negate(canonical))
	{
		SEMA_ERROR(expr->loc, "Cannot negate %s.", inner->type->name_loc.string);
		return false;
	}
	if (inner->expr_kind != EXPR_CONST)
	{
		expr->type = inner->type;
		return true;
	}
	// TODO UXX CAP
	expr_replace(expr, inner);
	switch (expr->const_expr.type)
	{
		case CONST_INT:
			expr->const_expr.i = ~expr->const_expr.i;
			break;
		case CONST_FLOAT:
			expr->const_expr.f = -expr->const_expr.i;
			break;
		default:
			UNREACHABLE
	}
	return true;
}
static bool sema_expr_analyse_bit_not(Context *context, Expr *expr, Expr *inner)
{
	Type *canonical = inner->type->canonical;
	if (!type_is_integer(canonical) && canonical != type_bool)
	{
		SEMA_ERROR(expr->loc, "Cannot bit negate %s.", inner->type->name_loc.string);
	}
	if (inner->expr_kind != EXPR_CONST)
	{
		expr->type = inner->type;
		return true;
	}
	expr_replace(expr, inner);
	// TODO UXX CAP
	switch (expr->const_expr.type)
	{
		case CONST_INT:
			expr->const_expr.i = ~expr->const_expr.i;
			break;
		case CONST_BOOL:
			expr->const_expr.b = !expr->const_expr.b;
			break;
		default:
			UNREACHABLE
	}
	return true;
}
static bool sema_expr_analyse_not(Context *context, Expr *expr, Expr *inner)
{
	if (inner->expr_kind == EXPR_CONST)
	{
		switch (expr->const_expr.type)
		{
			case CONST_NIL:
				expr->const_expr.b = true;
				break;
			case CONST_BOOL:
				expr->const_expr.b = !inner->const_expr.b;
				break;
			case CONST_INT:
				expr->const_expr.b = inner->const_expr.i == 0;
				break;
			case CONST_FLOAT:
				expr->const_expr.b = inner->const_expr.f == 0;
				break;
			case CONST_STRING:
				expr->const_expr.b = !inner->const_expr.string.len;
				break;
		}
		expr->const_expr.type = CONST_BOOL;
		expr->type = type_bool;
		expr->expr_kind = EXPR_CONST;
		return true;
	}
	Type *canonical = inner->type->canonical;
	switch (canonical->type_kind)
	{
		case TYPE_POISONED:
		case TYPE_IXX:
		case TYPE_UXX:
		case TYPE_FXX:
		case TYPE_INC_ARRAY:
		case TYPE_EXPRESSION:
			UNREACHABLE
		case TYPE_ARRAY:
		case TYPE_POINTER:
		case TYPE_VARARRAY:
		case TYPE_BOOL:
		case TYPE_I8:
		case TYPE_I16:
		case TYPE_I32:
		case TYPE_I64:
		case TYPE_U8:
		case TYPE_U16:
		case TYPE_U32:
		case TYPE_U64:
		case TYPE_F32:
		case TYPE_F64:
			return true;
		case TYPE_USER_DEFINED:
		case TYPE_VOID:
		case TYPE_STRING:
			SEMA_ERROR(expr->loc, "Cannot use 'not' on %s", inner->type->name_loc.string);
			return false;
	}
	UNREACHABLE
}

static bool sema_expr_analyse_preinc(Context *context, Expr *expr, Expr *inner) { TODO }
static bool sema_expr_analyse_predec(Context *context, Expr *expr, Expr *inner) { TODO }

static bool sema_expr_analyse_postinc(Context *context, Expr *expr, Expr *inner) { TODO }
static bool sema_expr_analyse_postdec(Context *context, Expr *expr, Expr *inner) { TODO }

static inline bool sema_expr_analyse_binary(Context *context, Expr *expr)
{
	assert(expr->resolve_status == RESOLVE_RUNNING);
	Expr *left = expr->binary_expr.left;
	Expr *right = expr->binary_expr.right;

	if (!sema_expr_analysis(context, left)) return false;
	if (!sema_expr_analysis(context, right)) return false;

	return BINOP_ANALYSIS[expr->binary_expr.operator](context, expr, left, right);
}

static inline bool sema_expr_analyse_unary(Context *context, Expr *expr)
{
	assert(expr->resolve_status == RESOLVE_RUNNING);
	Expr *inner = expr->unary_expr.expr;

	if (!sema_expr_analysis(context, inner)) return false;

	return UNARYOP_ANALYSIS[expr->unary_expr.operator](context, expr, inner);
}

static inline bool sema_expr_analyse_postunary(Context *context, Expr *expr)
{
	assert(expr->resolve_status == RESOLVE_RUNNING);
	Expr *inner = expr->post_expr.expr;

	if (!sema_expr_analysis(context, inner)) return false;

	return POSTUNARYOP_ANALYSIS[expr->post_expr.operator](context, expr, inner);
}

static inline bool sema_expr_analyse_try(Context *context, Expr *expr)
{
	if (!sema_expr_analysis(context, expr->try_expr.expr)) return false;
	expr->type = expr->try_expr.expr->type;
	if (expr->try_expr.else_expr)
	{
		if (!sema_expr_analysis(context, expr->try_expr.else_expr)) return false;
		if (!cast(expr->try_expr.else_expr, expr->type, CAST_TYPE_IMPLICIT)) return false;
	}
	// Check errors!
	TODO
	return true;
}

static inline bool sema_expr_analyse_type(Context *context, Expr *expr)
{
	TODO
	return true;
}


static ExprBinopAnalysis BINOP_ANALYSIS[TOKEN_EOF] = {
		[TOKEN_EQ] = &sema_expr_analyse_assign,
		[TOKEN_STAR] = &sema_expr_analyse_mult,
		[TOKEN_MULT_ASSIGN] = &sema_expr_analyse_mult_assign,
		[TOKEN_PLUS] = &sema_expr_analyse_add,
		[TOKEN_PLUS_ASSIGN] = &sema_expr_analyse_add_assign,
		[TOKEN_MINUS] = &sema_expr_analyse_sub,
		[TOKEN_MINUS_ASSIGN] = &sema_expr_analyse_sub_assign,
		[TOKEN_DIV] = &sema_expr_analyse_div,
		[TOKEN_DIV_ASSIGN] = &sema_expr_analyse_div_assign,
		[TOKEN_MOD] = &sema_expr_analyse_mod,
		[TOKEN_MOD_ASSIGN] = &sema_expr_analyse_mod_assign,
		[TOKEN_AND] = &sema_expr_analyse_and,
		[TOKEN_AND_ASSIGN] = &sema_expr_analyse_and_assign,
		[TOKEN_OR] = &sema_expr_analyse_bit_or,
		[TOKEN_OR_ASSIGN] = &sema_expr_analyse_or_assign,
		[TOKEN_AMP] = &sema_expr_analyse_bit_and,
		[TOKEN_BIT_AND_ASSIGN] = &sema_expr_analyse_bit_and_assign,
		[TOKEN_BIT_OR] = &sema_expr_analyse_bit_or,
		[TOKEN_BIT_OR_ASSIGN] = &sema_expr_analyse_bit_or_assign,
		[TOKEN_BIT_XOR] = &sema_expr_analyse_bit_xor,
		[TOKEN_BIT_XOR_ASSIGN] = &sema_expr_analyse_bit_xor_assign,
		[TOKEN_NOT_EQUAL] = &sema_expr_analyse_ne,
		[TOKEN_EQEQ] = &sema_expr_analyse_eq,
		[TOKEN_GREATER_EQ] = &sema_expr_analyse_ge,
		[TOKEN_GREATER] = &sema_expr_analyse_gt,
		[TOKEN_LESS_EQ] = &sema_expr_analyse_le,
		[TOKEN_LESS] = &sema_expr_analyse_lt,
		[TOKEN_SHR] = &sema_expr_analyse_shr,
		[TOKEN_SHR_ASSIGN] = &sema_expr_analyse_shr_assign,
		[TOKEN_SHL] = &sema_expr_analyse_shl,
		[TOKEN_SHL_ASSIGN] = &sema_expr_analyse_shl_assign,
		[TOKEN_ELVIS] = &sema_expr_analyse_elvis,
};


static ExprUnaryAnalysis UNARYOP_ANALYSIS[TOKEN_EOF + 1] = {
		[TOKEN_STAR] = &sema_expr_analyse_deref,
		[TOKEN_AMP] = &sema_expr_analyse_addr,
		[TOKEN_MINUS] = &sema_expr_analyse_neg,
		[TOKEN_BIT_NOT] = &sema_expr_analyse_bit_not,
		[TOKEN_NOT] = &sema_expr_analyse_not,
		[TOKEN_PLUSPLUS] = &sema_expr_analyse_preinc,
		[TOKEN_MINUSMINUS] = &sema_expr_analyse_predec,
};

static ExprUnaryAnalysis POSTUNARYOP_ANALYSIS[TOKEN_EOF + 1] = {
		[TOKEN_PLUSPLUS] = &sema_expr_analyse_postinc,
		[TOKEN_MINUSMINUS] = &sema_expr_analyse_postdec,
};


static ExprAnalysis EXPR_ANALYSIS[EXPR_CAST + 1] = {
		[EXPR_TRY] = &sema_expr_analyse_try,
		[EXPR_CONST] = NULL,
		[EXPR_BINARY] = &sema_expr_analyse_binary,
		[EXPR_CONDITIONAL] = &sema_expr_analyse_conditional,
		[EXPR_UNARY] = &sema_expr_analyse_unary,
		[EXPR_POST_UNARY] = &sema_expr_analyse_postunary,
		[EXPR_TYPE] = &sema_expr_analyse_type,
		[EXPR_IDENTIFIER] = &sema_expr_analyse_identifier,
		[EXPR_METHOD_REF] = &sema_expr_analyse_method_ref,
		[EXPR_CALL] = &sema_expr_analyse_call,
		[EXPR_SIZEOF] = &sema_expr_analyse_sizeof,
		[EXPR_SUBSCRIPT] = &sema_expr_analyse_subscript,
		[EXPR_ACCESS] = &sema_expr_analyse_access,
		[EXPR_STRUCT_VALUE] = &sema_expr_analyse_struct_value,
		[EXPR_STRUCT_INIT_VALUES] = &sema_expr_analyse_struct_init_values,
		[EXPR_INITIALIZER_LIST] = &sema_expr_analyse_initializer_list,
		[EXPR_CAST] = &sema_expr_analyse_cast,
};

bool sema_expr_analysis(Context *context, Expr *expr)
{
	switch (expr->resolve_status)
	{
		case RESOLVE_NOT_DONE:
			expr->resolve_status = RESOLVE_RUNNING;
			break;
		case RESOLVE_RUNNING:
			SEMA_ERROR(expr->loc, "Recursive resolution of expression");
			return expr_poison(expr);
		case RESOLVE_DONE:
			return expr_ok(expr);
	}
	if (!EXPR_ANALYSIS[expr->expr_kind](context, expr)) return expr_poison(expr);
	expr->resolve_status = RESOLVE_DONE;
	return true;
}