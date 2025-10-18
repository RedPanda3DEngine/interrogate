/**
 * PANDA 3D SOFTWARE
 * Copyright (c) Carnegie Mellon University.  All rights reserved.
 *
 * All use of this software is subject to the terms of the revised BSD
 * license.  You should have received a copy of this license along
 * with this source code in a file named "LICENSE."
 *
 * @file interfaceMakerCSharp.h
 * @author thetestgame
 * @date 2025-10-18
 */

#ifndef INTERFACEMAKERCSHARP_H
#define INTERFACEMAKERCSHARP_H

#include "dtoolbase.h"

#include "interfaceMaker.h"
#include "interrogate_interface.h"

class FunctionRemap;

/**
 * An InterfaceMaker suitable for generating C# P/Invoke wrappers and
 * managed class bindings for Panda class objects.
 */
class InterfaceMakerCSharp : public InterfaceMaker {
public:
  InterfaceMakerCSharp(InterrogateModuleDef *def);
  virtual ~InterfaceMakerCSharp();

  virtual void write_prototypes(std::ostream &out, std::ostream *out_h);
  virtual void write_functions(std::ostream &out);

  virtual ParameterRemap *remap_parameter(CPPType *struct_type, CPPType *param_type);

  virtual bool synthesize_this_parameter();

protected:
  virtual std::string get_wrapper_prefix();
  virtual std::string get_unique_prefix();

  virtual void
  record_function_wrapper(InterrogateFunction &ifunc,
                          FunctionWrapperIndex wrapper_index);

private:
  void write_prototype_for(std::ostream &out, Function *func);
  void write_function_for(std::ostream &out, Function *func);
  void write_function_instance(std::ostream &out, Function *func,
                               FunctionRemap *remap);
  void write_function_header(std::ostream &out, Function *func,
                             FunctionRemap *remap, bool newline);

  // C# specific helpers
  std::string map_type_to_csharp(CPPType *cpptype, bool is_return = false);
  void write_csharp_pinvoke_declaration(std::ostream &out, Function *func,
                                         FunctionRemap *remap);
};

#endif
