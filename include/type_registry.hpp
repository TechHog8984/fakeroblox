#pragma once

// CREDITS TO luau/Ast/include/Luau/Ast.h

extern int type_registry_index;

template<typename T>
struct TypeRegistration {
    static const int value;
};

template<typename T>
const int TypeRegistration<T>::value = ++type_registry_index;

#define REGISTER_TYPE(Class) \
    static int class_index() { \
        return TypeRegistration<Class>::value; \
    }
