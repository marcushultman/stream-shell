#pragma once

#include "closure.h"
#include "stream_parser.h"
#include "variant_ext.h"

using ExprValue = variant_ext_t<Value, Stream>;
using Expr = std::function<Result<ExprValue>(const Closure &)>;

using Operand = variant_ext_t<Value, Stream, StreamRef, Word, Expr>;

template <typename T>
concept IsValue = InVariant<Value, T>;

template <typename T>
concept IsExprValue = InVariant<ExprValue, T>;
