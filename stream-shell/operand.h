#pragma once

#include "closure.h"
#include "stream_parser.h"
#include "variant_ext.h"

using ClosureValue = std::function<Result<variant_ext_t<Value, Stream>>(const Closure &)>;
using Operand = variant_ext_t<Value, Stream, StreamRef, Word, ClosureValue>;

template <typename T>
concept ValueOrStream = InVariant<ClosureValue::result_type::value_type, T>;

template <typename T>
concept IsValue = ValueOrStream<T> && InVariant<Value, T>;
