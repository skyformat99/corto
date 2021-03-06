/* Copyright (c) 2010-2017 the corto developers
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <corto/corto.h>
#include "compare_ser.h"
#include "object.h"

#define CORTO_COMPARE(type,v1,v2) *(type*)v1 > *(type*)v2 ? CORTO_GT : *(type*)v1 < *(type*)v2 ? CORTO_LT : CORTO_EQ

static corto_int16 corto_ser_any(corto_walk_opt* s, corto_value *info, void *userData) {
    corto_any *this = corto_value_ptrof(info);
    corto_compare_ser_t *data = userData, privateData;
    corto_any *value = (void*)((corto_word)corto_value_ptrof(&data->value) + ((corto_word)this - (corto_word)data->base));

    corto_value v = corto_value_value(this->value, this->type);
    privateData.value = corto_value_value(value->value, value->type);

    /* Set base of privateData. Because we're reusing the serializer, the
     * construct callback won't be called again */
    privateData.base = this->value;

    corto_walk_value(s, &v, &privateData);

    data->result = privateData.result;

    return data->result != CORTO_EQ;
}

static corto_int16 corto_ser_primitive(corto_walk_opt* s, corto_value *info, void *userData) {
    corto_equalityKind result = CORTO_EQ;
    corto_compare_ser_t *data = userData;
    corto_type type = corto_value_typeof(info);
    void *this = corto_value_ptrof(info);
    void *value = (void*)((corto_word)corto_value_ptrof(&data->value) + ((corto_word)this - (corto_word)data->base));

    CORTO_UNUSED(s);

    switch(corto_primitive(type)->kind) {
    case CORTO_BOOLEAN:
        result = CORTO_COMPARE(corto_bool, this, value);
        break;
    case CORTO_CHARACTER:
        result = CORTO_COMPARE(corto_char, this, value);
        break;
    case CORTO_INTEGER:
    case CORTO_ENUM:
        switch(corto_primitive(type)->width) {
        case CORTO_WIDTH_8:
            result = CORTO_COMPARE(corto_int8, this, value);
            break;
        case CORTO_WIDTH_16:
            result = CORTO_COMPARE(corto_int16, this, value);
            break;
        case CORTO_WIDTH_32:
            result = CORTO_COMPARE(corto_int32, this, value);
            break;
        case CORTO_WIDTH_64:
            result = CORTO_COMPARE(corto_int64, this, value);
            break;
        case CORTO_WIDTH_WORD:
            result = CORTO_COMPARE(intptr_t, this, value);
            break;
        }
        break;
    case CORTO_BINARY:
    case CORTO_UINTEGER:
    case CORTO_BITMASK:
        switch(corto_primitive(type)->width) {
        case CORTO_WIDTH_8:
            result = CORTO_COMPARE(corto_uint8, this, value);
            break;
        case CORTO_WIDTH_16:
            result = CORTO_COMPARE(corto_uint16, this, value);
            break;
        case CORTO_WIDTH_32:
            result = CORTO_COMPARE(corto_uint32, this, value);
            break;
        case CORTO_WIDTH_64:
            result = CORTO_COMPARE(corto_uint64, this, value);
            break;
        case CORTO_WIDTH_WORD:
            result = CORTO_COMPARE(uintptr_t, this, value);
            break;
        }
        break;
    case CORTO_FLOAT:
        switch(corto_primitive(type)->width) {
        case CORTO_WIDTH_32:
            result = CORTO_COMPARE(corto_float32, this, value);
            break;
        case CORTO_WIDTH_64:
            result = CORTO_COMPARE(corto_float64, this, value);
            break;
        default:
            break;
        }
        break;
    case CORTO_TEXT: {
        corto_string thisStr = *(corto_string*)this;
        corto_string valueStr = *(corto_string*)value;
        if (thisStr && valueStr) {
            result = strcmp(thisStr, valueStr);
            if (result < 0) {
                result = CORTO_LT;
            } else if (result > 0) {
                result = CORTO_GT;
            }
        } else {
            if (thisStr == valueStr) {
                result = CORTO_EQ;
            } else {
                result = thisStr ? CORTO_GT : CORTO_LT;
            }
        }
        break;
    }
    }

    data->result = result;

    return data->result != CORTO_EQ;
}

static corto_int16 corto_ser_reference(corto_walk_opt* s, corto_value *info, void *userData) {
    corto_compare_ser_t *data = userData;
    void *this = corto_value_ptrof(info);
    void *value = (void*)((corto_word)corto_value_ptrof(&data->value) + ((corto_word)this - (corto_word)data->base));
    CORTO_UNUSED(s);

    if (*(corto_object*)this != *(corto_object*)value) {
        data->result = CORTO_NEQ;
    } else {
        data->result = CORTO_EQ;
    }

    return data->result != CORTO_EQ;
}

static corto_equalityKind corto_collection_compareArrayWithList(corto_collection t, void *array, corto_uint32 elementSize, corto_ll list) {
    corto_equalityKind result = CORTO_EQ;
    corto_uint32 i=0;
    corto_iter iter;
    void *e1, *e2;
    corto_type elementType = t->elementType;

    iter = corto_ll_iter(list);
    while(corto_iter_hasNext(&iter)) {
        if (corto_collection_requiresAlloc(elementType)) {
            e1 = corto_iter_next(&iter);
        } else {
            e1 = corto_iter_nextPtr(&iter);
        }
        e2 = CORTO_OFFSET(array, elementSize * i);
        result = corto_ptr_compare(e2, elementType, e1);
        if (result != CORTO_EQ) {
            break;
        }
        i++;
    }

    return result;
}

static corto_equalityKind corto_collection_compareListWithList(corto_collection t, corto_ll list1, corto_ll list2) {
    corto_equalityKind result = CORTO_EQ;
    corto_iter iter1, iter2;
    void *e1, *e2;
    corto_type elementType = t->elementType;

    iter1 = corto_ll_iter(list1);
    iter2 = corto_ll_iter(list2);
    while(corto_iter_hasNext(&iter1) && corto_iter_hasNext(&iter2)) {
        if (corto_collection_requiresAlloc(elementType)) {
            e1 = corto_iter_next(&iter1);
            e2 = corto_iter_next(&iter2);
        } else {
            e1 = corto_iter_nextPtr(&iter1);
            e2 = corto_iter_nextPtr(&iter2);
        }
        result = corto_ptr_compare(e1, elementType, e2);
        if (result != CORTO_EQ) {
            break;
        }
    }

    return result;
}

/* Compare composites */
static corto_int16 corto_ser_composite(corto_walk_opt* s, corto_value *info, void* userData) {
    corto_compare_ser_t *data = userData;
    corto_type t = corto_value_typeof(info);
    void *this = corto_value_ptrof(info);
    void *value = (void*)((corto_word)corto_value_ptrof(&data->value) + ((corto_word)this - (corto_word)data->base));

    if (corto_interface(t)->kind == CORTO_UNION) {
        corto_int32 d1 = *(corto_int32*)this;
        corto_int32 d2 = *(corto_int32*)value;

        if (d1 != d2) {
            corto_member m1 = safe_corto_union_findCase(t, d1);
            corto_member m2 = safe_corto_union_findCase(t, d2);
            if (m1 != m2) {
                goto nomatch;
            }
        }
    }

    return corto_walk_members(s, info, userData);
nomatch:
    data->result = CORTO_NEQ;
    return 1;
}

static corto_uint32 corto_ser_getCollectionSize(corto_any this) {
    corto_uint32 result = 0;
    switch(corto_collection(this.type)->kind) {
    case CORTO_ARRAY:
        result = corto_collection(this.type)->max;
        break;
    case CORTO_SEQUENCE:
        result = ((corto_objectseq*)this.value)->length;
        break;
    case CORTO_LIST:
        result = corto_ll_size(*(corto_ll*)this.value);
        break;
    case CORTO_MAP:
        result = corto_rb_size(*(corto_rbtree*)this.value);
        break;
    }
    return result;
}

/* Compare collections */
static corto_int16 corto_ser_collection(corto_walk_opt* s, corto_value *info, void* userData) {
    corto_type t1, t2;
    void *v1, *v2;
    corto_equalityKind result = CORTO_EQ;
    corto_uint32 size1 = 0, size2 = 0;
    corto_compare_ser_t *data = userData;
    corto_any a1, a2;

    CORTO_UNUSED(s);

    /* If this function is reached, collection-types are either equal or comparable. When the
     * base-object was a collection, the collection type can be different. When the base-object
     * was a composite type, the collection type has to be equal, since different composite
     * types are considered non-comparable. */
    t1 = corto_value_typeof(info);
    v1 = corto_value_ptrof(info);

    /* Verify whether current serialized object is the base-object */
    if (info->parent) {
        t2 = t1;
        v2 = (void*)((corto_word)corto_value_ptrof(&data->value) + ((corto_word)v1 - (corto_word)data->base));
    } else {
        t2 = corto_value_typeof(&data->value);
        v2 = corto_value_ptrof(&data->value);
    }

    /* Prepare any structures for determining sizes of collections */
    a1.type = t1;
    a1.value = v1;
    a2.type = t2;
    a2.value = v2;

    if ((size1 = corto_ser_getCollectionSize(a1)) == (size2 = corto_ser_getCollectionSize(a2))) {
        void *array1=NULL, *array2=NULL;
        corto_ll list1=NULL, list2=NULL;
        corto_uint32 elementSize=0, mem=0;

        elementSize = corto_type_sizeof(corto_collection(t1)->elementType);

        switch(corto_collection(t1)->kind) {
            case CORTO_ARRAY:
                array1 = v1;
                elementSize = corto_type_sizeof(corto_collection(t1)->elementType);
                mem = corto_collection(t1)->max * elementSize;
                break;
            case CORTO_SEQUENCE:
                array1 = ((corto_objectseq*)v1)->buffer;
                elementSize = corto_type_sizeof(corto_collection(t1)->elementType);
                mem = ((corto_objectseq*)v1)->length * elementSize;
                break;
            case CORTO_LIST:
                list1 = *(corto_ll*)v1;
                break;
            case CORTO_MAP:
                break;
        }

        switch(corto_collection(t2)->kind) {
            case CORTO_ARRAY:
                array2 = v2;
                break;
            case CORTO_SEQUENCE:
                array2 = ((corto_objectseq*)v2)->buffer;
                break;
            case CORTO_LIST:
                list2 = *(corto_ll*)v2;
                break;
            case CORTO_MAP:
                break;
        }

        if (array1) {
            if (array2) {
                result = memcmp(array1,array2,mem); /* TODO: do a serialized compare */
            } else if (list2) {
                result = corto_collection_compareArrayWithList(corto_collection(t1), array1, elementSize, list2);
            }
        } else if (list1) {
            if (array2) {
                result = corto_collection_compareArrayWithList(corto_collection(t1), array2, elementSize, list1);
                /* Reverse result */
                if (result != CORTO_EQ) {
                    if (result == CORTO_GT) {
                        result = CORTO_LT;
                    } else {
                        result = CORTO_GT;
                    }
                }
            } else if (list2) {
                result = corto_collection_compareListWithList(corto_collection(t1), list1, list2);
            }
        }
    } else {
        result = size1 > size2 ? CORTO_GT : CORTO_LT;
    }

    data->result = result < 0 ? -1 : result > 0 ? 1 : 0;

    return data->result != CORTO_EQ;
}

static corto_int16 corto_ser_construct(corto_walk_opt* s, corto_value *info, void *userData) {
    corto_compare_ser_t *data = userData;
    corto_bool compare = FALSE;
    CORTO_UNUSED(s);

    corto_type t1 = corto_value_typeof(info);
    corto_type t2 = corto_value_typeof(&data->value);

    /* If types are different, validate whether comparison should take place */
    if (t1 != t2) {
        /* Certain types of collections can be compared with each other as long as their elementTypes
         * are equal */
        if ((t1->kind == CORTO_COLLECTION) && (t1->kind == t2->kind)) {
            switch(corto_collection(t1)->kind) {
            case CORTO_ARRAY:
            case CORTO_SEQUENCE:
            case CORTO_LIST:
                switch(corto_collection(t2)->kind) {
                case CORTO_ARRAY:
                case CORTO_SEQUENCE:
                case CORTO_LIST:
                    if (corto_collection(t1)->elementType == corto_collection(t2)->elementType) {
                        compare = TRUE;
                    }
                    break;
                case CORTO_MAP:
                    compare = FALSE;
                    break;
                }
                break;
            case CORTO_MAP:
                if (corto_collection(t2)->kind == CORTO_MAP) {
                    if (corto_collection(t1)->elementType == corto_collection(t2)->elementType) {
                        if (corto_map(t1)->keyType == corto_map(t2)->keyType) {
                            compare = TRUE;
                        }
                    }
                }
                break;
            }
        } else {
            if ((!t1->reference && (t1->kind == CORTO_VOID)) && (t2->reference ||
               ((t2->kind == CORTO_PRIMITIVE) && (corto_primitive(t2)->kind == CORTO_TEXT)))) {
                compare = TRUE;
            }
            if ((!t2->reference && (t2->kind == CORTO_VOID)) && (t1->reference ||
               ((t1->kind == CORTO_PRIMITIVE) && (corto_primitive(t1)->kind == CORTO_TEXT)))) {
                compare = TRUE;
            }
        }
    } else {
        compare = TRUE;
    }

    data->result = compare ? CORTO_EQ : CORTO_NEQ;
    data->base = corto_value_ptrof(info);

    return !compare;
}

corto_walk_opt corto_compare_ser(corto_modifier access, corto_operatorKind accessKind, corto_walk_traceKind trace) {
    corto_walk_opt s;

    corto_walk_init(&s);

    s.access = access;
    s.accessKind = accessKind;
    s.traceKind = trace;
    s.construct = corto_ser_construct;
    s.program[CORTO_VOID] = NULL;
    s.program[CORTO_ANY] = corto_ser_any;
    s.program[CORTO_PRIMITIVE] = corto_ser_primitive;
    s.program[CORTO_COMPOSITE] = corto_ser_composite;
    s.program[CORTO_COLLECTION] = corto_ser_collection;
    s.reference = corto_ser_reference;

    return s;
}
