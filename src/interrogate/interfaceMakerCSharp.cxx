/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file interfaceMakerCSharp.cxx
 * @author thetestgame
 * @date 2025-10-18
 */

#include "interfaceMakerCSharp.h"
#include "interrogateBuilder.h"
#include "interrogate.h"
#include "functionRemap.h"
#include "parameterRemapHandleToInt.h"
#include "parameterRemapUnchanged.h"
#include "typeManager.h"

#include "interrogateDatabase.h"
#include "interrogateType.h"
#include "interrogateFunction.h"
#include "cppFunctionType.h"
#include "cppPointerType.h"
#include "cppConstType.h"
#include "cppReferenceType.h"

using std::ostream;
using std::string;

/**
 * Constructor
 */
InterfaceMakerCSharp::
InterfaceMakerCSharp(InterrogateModuleDef *def) :
  InterfaceMaker(def)
{
}

/**
 * Destructor
 */
InterfaceMakerCSharp::
~InterfaceMakerCSharp() {
}

/**
 * Generates the list of function prototypes corresponding to the functions
 * that will be output in write_functions().
 */
void InterfaceMakerCSharp::
write_prototypes(ostream &out, ostream *out_h) {
  // Write C export declarations for the native library
  out <<
    "#if __GNUC__ >= 4\n"
    "#define EXPORT_FUNC extern \"C\" __attribute__((used, visibility(\"default\")))\n"
    "#elif defined(_MSC_VER)\n"
    "#define EXPORT_FUNC extern \"C\" __declspec(dllexport)\n"
    "#else\n"
    "#define EXPORT_FUNC extern \"C\"\n"
    "#endif\n\n";

  FunctionsByIndex::iterator fi;
  for (fi = _functions.begin(); fi != _functions.end(); ++fi) {
    Function *func = (*fi).second;
    write_prototype_for(out, func);
  }

  // If we have an output header file, write C# P/Invoke declarations
  if (out_h != nullptr) {
    *out_h << "using System;\n";
    *out_h << "using System.Runtime.InteropServices;\n\n";
    *out_h << "namespace Panda3D\n";
    *out_h << "{\n";
    *out_h << "  internal static class NativeMethods\n";
    *out_h << "  {\n";
    *out_h << "    private const string DllName = \"libinterrogate\";\n\n";

    for (fi = _functions.begin(); fi != _functions.end(); ++fi) {
      Function *func = (*fi).second;
      Function::Remaps::const_iterator ri;
      for (ri = func->_remaps.begin(); ri != func->_remaps.end(); ++ri) {
        FunctionRemap *remap = (*ri);
        if (remap->_extension || (remap->_flags & FunctionRemap::F_explicit_self)) {
          continue;
        }
        write_csharp_pinvoke_declaration(*out_h, func, remap);
      }
    }

    *out_h << "  }\n";
    *out_h << "}\n";
  }

  out << "\n";
  InterfaceMaker::write_prototypes(out, out_h);
}

/**
 * Generates the list of functions that are appropriate for this interface.
 * This function is called *before* write_prototypes(), above.
 */
void InterfaceMakerCSharp::
write_functions(ostream &out) {
  FunctionsByIndex::iterator fi;
  for (fi = _functions.begin(); fi != _functions.end(); ++fi) {
    Function *func = (*fi).second;
    write_function_for(out, func);
  }

  InterfaceMaker::write_functions(out);
}

/**
 * Allocates a new ParameterRemap object suitable to the indicated parameter
 * type.  If struct_type is non-NULL, it is the type of the enclosing class
 * for the function (method) in question.
 *
 * The return value is a newly-allocated ParameterRemap object, if the
 * parameter type is acceptable, or NULL if the parameter type cannot be
 * handled.
 */
ParameterRemap *InterfaceMakerCSharp::
remap_parameter(CPPType *struct_type, CPPType *param_type) {
  // Wrap TypeHandle and ButtonHandle as integers for easier interop
  if (TypeManager::is_handle(param_type)) {
    return new ParameterRemapHandleToInt(param_type);
  } else {
    return InterfaceMaker::remap_parameter(struct_type, param_type);
  }
}

/**
 * This method should be overridden and redefined to return true for
 * interfaces that require the implicit "this" parameter, if present, to be
 * passed as the first parameter to any wrapper functions.
 */
bool InterfaceMakerCSharp::
synthesize_this_parameter() {
  return true;
}

/**
 * Returns the prefix string used to generate wrapper function names.
 */
string InterfaceMakerCSharp::
get_wrapper_prefix() {
  return "_inCS";
}

/**
 * Returns the prefix string used to generate unique symbolic names, which are
 * not necessarily C-callable function names.
 */
string InterfaceMakerCSharp::
get_unique_prefix() {
  return "csharp";
}

/**
 * Associates the function wrapper with its function in the appropriate
 * structures in the database.
 */
void InterfaceMakerCSharp::
record_function_wrapper(InterrogateFunction &ifunc,
                        FunctionWrapperIndex wrapper_index) {
  // We'll store C# wrappers in the same place as C wrappers since the
  // calling convention is compatible
  ifunc._c_wrappers.push_back(wrapper_index);
}

/**
 * Writes the prototype for the indicated function.
 */
void InterfaceMakerCSharp::
write_prototype_for(ostream &out, InterfaceMaker::Function *func) {
  Function::Remaps::const_iterator ri;

  for (ri = func->_remaps.begin(); ri != func->_remaps.end(); ++ri) {
    FunctionRemap *remap = (*ri);

    if (remap->_extension || (remap->_flags & FunctionRemap::F_explicit_self)) {
      continue;
    }

    if (output_function_names) {
      out << "EXPORT_FUNC ";
    }
    write_function_header(out, func, remap, false);
    out << ";\n";
  }
}

/**
 * Writes the definition for a function that will call the indicated C++
 * function or method.
 */
void InterfaceMakerCSharp::
write_function_for(ostream &out, InterfaceMaker::Function *func) {
  Function::Remaps::const_iterator ri;

  for (ri = func->_remaps.begin(); ri != func->_remaps.end(); ++ri) {
    FunctionRemap *remap = (*ri);
    write_function_instance(out, func, remap);
  }
}

/**
 * Writes out the particular function that handles a single instance of an
 * overloaded function.
 */
void InterfaceMakerCSharp::
write_function_instance(ostream &out, InterfaceMaker::Function *func,
                        FunctionRemap *remap) {
  if (remap->_extension || (remap->_flags & FunctionRemap::F_explicit_self)) {
    return;
  }

  out << "/*\n"
      << " * C# wrapper for\n"
      << " * ";
  remap->write_orig_prototype(out, 0, false, remap->_num_default_parameters);
  out << "\n"
      << " */\n";

  if (!output_function_names) {
    // If we're not saving the function names, don't export it from the
    // library.
    out << "static ";
  }

  write_function_header(out, func, remap, true);
  out << " {\n";

  if (generate_spam) {
    write_spam_message(out, remap);
  }

  string return_expr =
    remap->call_function(out, 2, true, "param0");
  return_expr = manage_return_value(out, 2, remap, return_expr);
  if (!return_expr.empty()) {
    out << "  return " << return_expr << ";\n";
  }

  out << "}\n\n";
}

/**
 * Writes the first line of a function definition, either for a prototype or a
 * function body.
 */
void InterfaceMakerCSharp::
write_function_header(ostream &out, InterfaceMaker::Function *func,
                      FunctionRemap *remap, bool newline) {
  if (remap->_void_return) {
    out << "void";
  } else {
    out << remap->_return_type->get_new_type()->get_local_name(&parser);
  }
  if (newline) {
    out << "\n";
  } else {
    out << " ";
  }

  out << remap->_wrapper_name << "(";
  int pn = 0;
  if (pn < (int)remap->_parameters.size()) {
    remap->_parameters[pn]._remap->get_new_type()->
      output_instance(out, remap->get_parameter_name(pn), &parser);
    pn++;
    while (pn < (int)remap->_parameters.size()) {
      out << ", ";
      remap->_parameters[pn]._remap->get_new_type()->
        output_instance(out, remap->get_parameter_name(pn), &parser);
      pn++;
    }
  }
  out << ")";
}

/**
 * Maps a C++ type to its corresponding C# type for P/Invoke declarations.
 */
string InterfaceMakerCSharp::
map_type_to_csharp(CPPType *cpptype, bool is_return) {
  if (cpptype == nullptr) {
    return "void";
  }

  // Unwrap const and reference types
  CPPType *unwrapped = cpptype;
  while (unwrapped != nullptr) {
    CPPConstType *const_type = unwrapped->as_const_type();
    if (const_type != nullptr) {
      unwrapped = const_type->_wrapped_around;
      continue;
    }
    CPPReferenceType *ref_type = unwrapped->as_reference_type();
    if (ref_type != nullptr) {
      unwrapped = ref_type->_pointing_at;
      continue;
    }
    break;
  }

  // Check for pointer types
  CPPPointerType *ptr_type = unwrapped->as_pointer_type();
  if (ptr_type != nullptr) {
    CPPType *pointed_at = ptr_type->_pointing_at;

    // char* maps to string in C# (with marshaling)
    if (TypeManager::is_char(pointed_at)) {
      return "IntPtr"; // Use IntPtr for safety, marshal with MarshalAs if needed
    }

    // Other pointers map to IntPtr
    return "IntPtr";
  }

  // Get the basic type name
  string type_name = unwrapped->get_local_name(&parser);

  // Map common C++ types to C# types
  if (type_name == "void") {
    return "void";
  } else if (type_name == "bool") {
    return "bool";
  } else if (type_name == "char") {
    return "byte";
  } else if (type_name == "signed char") {
    return "sbyte";
  } else if (type_name == "unsigned char") {
    return "byte";
  } else if (type_name == "short" || type_name == "short int") {
    return "short";
  } else if (type_name == "unsigned short" || type_name == "unsigned short int") {
    return "ushort";
  } else if (type_name == "int") {
    return "int";
  } else if (type_name == "unsigned int") {
    return "uint";
  } else if (type_name == "long" || type_name == "long int") {
    return "int"; // On Windows, long is 32-bit
  } else if (type_name == "unsigned long" || type_name == "unsigned long int") {
    return "uint";
  } else if (type_name == "long long" || type_name == "long long int") {
    return "long";
  } else if (type_name == "unsigned long long" || type_name == "unsigned long long int") {
    return "ulong";
  } else if (type_name == "float") {
    return "float";
  } else if (type_name == "double") {
    return "double";
  } else if (type_name == "size_t") {
    return "UIntPtr";
  } else if (type_name == "ptrdiff_t") {
    return "IntPtr";
  }

  // Default to IntPtr for unknown types (handles, structs, etc.)
  return "IntPtr";
}

/**
 * Writes a C# P/Invoke declaration for the given function.
 */
void InterfaceMakerCSharp::
write_csharp_pinvoke_declaration(ostream &out, Function *func,
                                  FunctionRemap *remap) {
  out << "    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]\n";

  // Map return type
  string return_type;
  if (remap->_void_return) {
    return_type = "void";
  } else {
    return_type = map_type_to_csharp(remap->_return_type->get_new_type(), true);
  }

  out << "    internal static extern " << return_type << " "
      << remap->_wrapper_name << "(";

  // Map parameters
  int pn = 0;
  if (pn < (int)remap->_parameters.size()) {
    string param_type = map_type_to_csharp(
      remap->_parameters[pn]._remap->get_new_type(), false);
    out << param_type << " " << remap->get_parameter_name(pn);
    pn++;
    while (pn < (int)remap->_parameters.size()) {
      out << ", ";
      param_type = map_type_to_csharp(
        remap->_parameters[pn]._remap->get_new_type(), false);
      out << param_type << " " << remap->get_parameter_name(pn);
      pn++;
    }
  }
  out << ");\n\n";
}
