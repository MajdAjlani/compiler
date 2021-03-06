#pragma once
#ifndef ALL_TYPE_EXP
#define ALL_TYPE_EXP
#include "TypeExpression.h"
// simple type
#define TYPE_ERROR 0
#define TYPE_VOID 1
#define TYPE_BOOL 2
#define TYPE_INTEGER 3
#define TYPE_FLOAT 4
#define TYPE_DOUBLE 5
#define TYPE_CHAR 6
#define TYPE_STRING 7

#include "TypeBoolean.h"
#include "TypeFloat.h"
#include "TypeDouble.h"
#include "TypeInteger.h"
#include "TypeVoid.h"
#include "TypeString.h"
#include "TypeError.h"

// complex type
#define TYPE_ARRAY 8
#define TYPE_FUNCTION 9
#define TYPE_CLASS 10

#include "TypeArray.h"
#include "TypeFunction.h"
#include "TypeClass.h"


// Helpers and Aux
#include "TypeCheckingHelper.h"
#include "TypesTable.h"

// Code Generator helper class
#include "../Code Generator/AsmGenerator.h"

#endif