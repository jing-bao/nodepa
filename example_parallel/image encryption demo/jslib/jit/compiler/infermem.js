/*
 * Copyright (c) 2011, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice, 
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

if (RiverTrail === undefined) {
    var RiverTrail = {};
}

RiverTrail.InferMem = function () {
    var definitions = Narcissus.definitions;
    eval(definitions.consts);
    eval(RiverTrail.definitions.consts);

    const debug = false;

    // A MemSet models a set of memory variables for a given size. Optinally,
    // each memory name might be associated with a set of aliases that may
    // refer to the same memory. Such aliases are created when two memory sets
    // are overlayed, e.g., because they correspond to two independent control
    // flow paths.
    var MemSet = function () {
        var unique = {value : 0};
        return function () {
            this._store = {};
            this._store.__proto__ = null;
            this._unique = unique;
            return this;
        };
    }();
    var MSP = MemSet.prototype = {};
    MSP.create = function create (name) {
        var memName = "_mem_" + name + "_" + this._unique.value++;
        this.add(memName);
        return memName;
    }
    MSP.add = function add (name) {
        this._store[name] = null;
    }
    MSP.union = function union (other) {
        for (var mem in other._store) {
            if (other._store[mem] !== null) {
                // this entry has a set of aliases attached to it, so copy
                this._store[mem] = new MemSet();
                this._store[mem].union(other._store[mem]);
            } else {
                this.add(mem);
            }
        }
    };
    MSP.overlay = function overlap (other) {
        var keys = Object.keys(this._store);
        var keyPos = 0;
        for (var mem in other._store) {
            var ms;
            if (keyPos < keys.length) {
                if (this._store[keys[keyPos]] === null) {
                    this._store[keys[keyPos]] = new MemSet();
                }
                ms = this._store[keys[keyPos]];
                ms.add(mem);
            } else {
                this.add(mem);
                ms = this._store[mem] = new MemSet();
            }
            if (other._store[mem] !== null) {
                for (var alias in other._store[mem]._store) {
                    ms.add(alias);
                }
            }
            keyPos++;
        }
    };
    MSP.declare = function declare (size) {
        var result = "";
        for (var name in this._store) {
            result += "char " + name + "[" + size + "];";
            if (this._store[name] !== null) {
                result += this._store[name].declareAlias(name);
            }
        };
        return result;
    };
    MSP.declareAlias = function declareAlias (alias) {
        result = "";
        for (var name in this._store) {
            result += "char *" + name + " = " + alias + ";";
        }
        return result;
    }

    // A memory list models a mapping from memory sizes to memory variables.
    // I sort them by sizes so that I can more easily overlay different
    // memory lists.
    var MemList = function () {
        this._store = [];
        return this;
    };
    var MLP = MemList.prototype = {};
    MLP.allocate = function allocate (size, name) {
        if (this._store[size] === undefined) {
            this._store[size] = new MemSet();
        }
        return this._store[size].create(name);
    };
    MLP.join = function join (other) {
        for (var size in other._store) {
            if (this._store[size] === undefined) {
                this._store[size] = new MemSet();
            }
            this._store[size].union(other._store[size]);
        }
    };
    MLP.overlay = function overlay (other) {
        for (var size in other._store) {
            if (this._store[size] === undefined) {
                this._store[size] = new MemSet();
            }
            this._store[size].overlay(other._store[size]);
        }
    };
    MLP.declare = function declare () {
        var result = "";
        for (var size in this._store) {
            result += this._store[size].declare(size);
        }
        return result;
    };

    //
    // error reporting
    //
    function reportError(msg, t) {
        throw "Error: " + msg + " <" + (t ? RiverTrail.Helper.wrappedPP(t) : "no context") + ">"; // could be more elaborate
    }
    function reportBug(msg, t) {
        throw "Bug: " + msg; // could be more elaborate
    }

    var isArrayLiteral = RiverTrail.Helper.isArrayLiteral;

    // The code below creates a single buffer for each
    // dimension of the nested array. These buffers are
    // attached to the AST node and the backend emits code
    // for initializing the pointers in each of these
    // buffers.
    function allocateArrayMem(ast, memVars) {
        var shape = ast.typeInfo.getOpenCLShape();
        var shape_len = shape.length;
        if(shape_len === 1) {
            ast.allocatedMem = memVars.allocate(ast.typeInfo.getOpenCLSize(), "CALL");
        }
        else {
            ast.memBuffers = {size:0, list:[]};
            var redu = 1;
            for(var i = 0; i < shape_len; i++) {
                //var type_size = getTypeSize(i, shape, ast.typeInfo.OpenCLType);
                var type_size = RiverTrail.Helper.getOpenCLSize(ast.typeInfo.OpenCLType);
                var allocation_size = type_size*shape[i]*redu;
                //debug && console.log("Allocating " + allocation_size + " bytes in " +  "CALL_"
                //        + i + "  for i = " + i);
                var memBufferName = memVars.allocate(allocation_size, "CALL_" + i);
                ast.memBuffers.size +=1;
                ast.memBuffers.list.push(memBufferName);

                redu = redu*shape[i];
            }
            // Set the primary memory buffer for this node to be the
            // top-level buffer
            ast.allocatedMem = ast.memBuffers.list[0];
            //debug && console.log("Total AST allocations: ", ast.memBuffers.size, ast.memBuffers.list.length);
        }
    }


    // We allocate memory for the fields of the object
    // and for the object itself (pointer to the fields)
    function allocateObjMem(ast, memVars) {
        var fields = ast.typeInfo.properties.fields;
        var objSize = 0;
        ast.memBuffers = {__size:0, __root:null};
        for(var idx in fields) {
            ast.memBuffers[idx] = [];
            if(fields[idx].isScalarType()) {
                var allocation_size = RiverTrail.Helper.getOpenCLSize(fields[idx].OpenCLType);
                objSize += allocation_size;
            }
            else if (fields[idx].name === "InlineObject") {
                reportError("InlineObject type properties not implemented yet");
            }
            else if(fields[idx].name === "Array") {
                var shape = fields[idx].getOpenCLShape();
                var shape_len = shape.length;

                objSize += RiverTrail.Helper.getOpenCLSize(fields[idx].OpenCLType);
                var redu = 1;
                for(var i = 0; i < shape_len; i++) {
                    //var type_size = getTypeSize(i, shape, ast.typeInfo.OpenCLType);
                    var type_size = RiverTrail.Helper.getOpenCLSize(fields[idx].OpenCLType);
                    var allocation_size = type_size*shape[i]*redu;
                    debug && console.log("Allocating " + allocation_size + " bytes in " +  "FLD_" + i + "  for i = " + i);
                    var memBufferName = memVars.allocate(allocation_size, "FLD_" + i);
                    ast.memBuffers.__size +=1;
                    ast.memBuffers[idx].push(memBufferName);
                    redu = redu*shape[i];
                }
            }
            else {
                reportError("Unknown field type");
            }
        }
        // Allocate space for the fields of the object
        var obj_memBufferName = memVars.allocate(objSize, "OBJ");
        ast.memBuffers.size += 1;
        ast.memBuffers.__root = obj_memBufferName;
        ast.allocatedMem = obj_memBufferName;
    }

    function infer(ast, memVars, ins, outs) {
        "use strict";

        switch (ast.type) {
            case 42:
                ast.funDecls.forEach(function (f) {infer(f.body);});
                ast.memVars = new MemList();
                ast.children.forEach(function (child) { infer(child, ast.memVars, null, null); });
                break;

            case 43:
                ast.children.forEach(function (child) { infer(child, memVars, ins, outs); });
                break;

            //
            // statements
            //
            case 77:
                // this is not an applied occurence but the declaration, so we do not do anything here
                break;
            case 84:
                // special case: if the value is an ARRAY_INIT that only contains allocation free
                // expressions, we do not allocate space for the frame as it is directly written
                if (!isArrayLiteral(ast.value)) {
                    infer(ast.value, memVars, ins, outs);
                }
                break;
            //
            // loops
            //
            case 72:
                // fallthrough;
            case 76:
                // setup is run once
                if (ast.setup) {
                    infer(ast.setup, memVars, ins, outs);
                }
                // fallthrough;
            case 94:
                infer(ast.condition, memVars, ins, outs);
                infer(ast.body, memVars, ast.ins, ast.outs);
                if (ast.update) {
                    infer(ast.update, memVars, ast.ins, ast.outs);
                }
                break;
            case 78:
                infer(ast.condition, memVars, ins, outs);
                var thenMem = new MemList();
                infer(ast.thenPart, thenMem, ins, outs);
                if (ast.elsePart) {
                    var elseMem = new MemList();
                    infer(ast.elsePart, elseMem, ins, outs);
                    thenMem.overlay(elseMem);
                }
                memVars.join(thenMem);
                break;
            case 2:
                if (ast.expression) {
                    infer(ast.expression, memVars, ins, outs);
                }
                break;
            case 91:
            case 67:
                ast.children.forEach(function (ast) {
                                         if (ast.initializer) {
                                             infer(ast.initializer, memVars, ins, outs);
                                         }
                                     });
                break;
            case 4:
                // children[0] is the left hand side, children[1] is the right hand side.
                // both can be expressions. 
                infer(ast.children[0], memVars, ins, outs);
                infer(ast.children[1], memVars, ins, outs);
                switch (ast.children[0].type) {
                    case 60:
                        // a = expr
                        //
                        // case 1:
                        // If <expr> is in the __private address space, then if <a> is an in and out var we have to copy, 
                        // as the memory we have allocated for <expr> could potentially be reused in the next iteration 
                        // of the loop before <a> has been read.
                        //
                        // case 2:
                        // If <expr> is in a different address space than <a>, we have to copy, too.
                        var aVar = ast.children[0];
                        if (((ast.children[1].typeInfo.getOpenCLAddressSpace() === "__private") && // case 1
                            (ins && ins.contains(aVar.value) && outs && outs.contains(aVar.value))) ||
                            (aVar.typeInfo.getOpenCLAddressSpace() != ast.children[1].typeInfo.getOpenCLAddressSpace())) { // case 2
                            if(!ast.typeInfo.isScalarType()) {
                                var shape = ast.typeInfo.getOpenCLShape();
                                var shape_len = shape.length;
                                debug && console.log("Creating memory for " + ast.children[0].value + " with shape: ", shape);
                                ast.memBuffers = {size:0, list:[]};
                                var redu = 1;
                                for(var i = 0; i < shape_len; i++) {
                                    //var type_size = getTypeSize(i, shape, ast.typeInfo.OpenCLType);
                                    var type_size = RiverTrail.Helper.getOpenCLSize(ast.typeInfo.OpenCLType);
                                    var allocation_size = type_size*shape[i]*redu;
                                    debug && console.log("Allocating " + allocation_size + " bytes in " +  ast.children[0].value
                                      + "_" + i + "  for i = " + i);
                                    var memBufferName = memVars.allocate(allocation_size, ast.children[0].value + "_" + i);
                                    ast.memBuffers.size +=1;
                                    ast.memBuffers.list.push(memBufferName);

                                    redu = redu*shape[i];
                                }
                                // Set the primary memory buffer for this node to be the
                                // top-level buffer
                                ast.allocatedMem = ast.memBuffers.list[0];
                                debug && console.log("Total AST allocations: ", ast.memBuffers.size, ast.memBuffers.list.length); 
                            }
                        }
                        break;
                    case 48:
                        // case of a[iv] = expr. 
                        break;
                    case 35:
                        // Support for updates on object properties.
                        infer(ast.children[0], memVars, ins, outs);
                        infer(ast.children[1], memVars, ins, outs);
                        break;
                    default:
                        reportBug("unhandled lhs in assignment");
                        break;
                }
                break;
                
            // 
            // expressions
            //

            case 5:
                // the hook (?) is badly designed. The first child is the condition, second child
                // the then expression, third child the else expression
                infer(ast.children[0], memVars, ins, outs);
                var thenMem = new MemList();
                infer(ast.children[1], thenMem, ins, outs); 
                var elseMem = new MemList();
                infer(ast.children[2], elseMem, ins, outs);
                thenMem.overlay(elseMem);
                memVars.join(thenMem);
                break;
                
            // literals
            case 60:
            case 86:
            case 61:
            case 88:
            case 74:
                // nothing to do here
                break;

            case 49:
                // These require allocation
                if((ast.typeInfo.properties.shape.length === 1) && ast.typeInfo.properties.elements.isScalarType()) {
                    ast.allocatedMem = memVars.allocate(ast.typeInfo.getOpenCLSize(), "ARRAY_INIT");
                }
                else {
                    // If this is a nested array, we only need to allocate
                    // memory for the current outermost array expression.
                    ast.allocatedMem = memVars.allocate(RiverTrail.Helper.getOpenCLSize(ast.typeInfo.OpenCLType) * ast.typeInfo.properties.shape[0], "ARRAY_INIT");
                }
                // fallthrough;
                
            // stuff where we just look at the children
            case 3:
            case 33:
            case 24: 
            case 34:
            case 25:
            case 26:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
            case 18:
            case 19:
            case 20:
            case 10:
            case 11:
            case 12:
            case 21:
            case 22:
            case 23:
            case 27:
            case 28:    
            case 9: 
            case 8:
            case 29:
            case 31:
            case 32:
            case 30:
            case 35:
            case 48:
            case 55:      
            case 96:
            case 98:
            case 97:
                if (ast.children) {
                    ast.children.forEach( function (child) { infer(child, memVars, ins, outs); });
                }
                break;
            case 46: 
                if (ast.children) {
                    if(ast.children[0].type === 35 && ast.children[0].children[0].value === "RiverTrailUtils") {
                        switch (ast.children[0].children[1].value) {
                            case "createArray":
                                allocateArrayMem(ast, memVars);
                                break;
                            default:
                                reportError("Invalid method " + ast.children[0].children[1].value + " on RiverTrailUtils", ast);
                        }
                    }
                    else {
                        ast.children.forEach( function (child) { infer(child, memVars, ins, outs); });
                    }
                }
                if(ast.typeInfo.name === "InlineObject") {
                    allocateObjMem(ast, memVars);
                }
                // If I am returning an Array space needs to be allocated for it in the caller and 
                // the name of the space should be left in the CALL nodes allocatedMem field so that when
                // I generate the call it is available. However, if this method does return a pointer
                // to some existing data, like |get| on ParallelArray, the type inference will have
                // left an isShared annotation and no memory needs to be allocated.
                else if (!ast.typeInfo.isScalarType() && !ast.typeInfo.properties.isShared) { 
                    // This call returns a nested array. The caller needs to allocate enough
                    // memory for this array and initialize the pointers in
                    // the allocated buffer to create a structure that the
                    // callee can simply fill the leaves of.
                    //
                    allocateArrayMem(ast, memVars);
                }
                break;

            // 
            // unsupported stuff here
            //
            case 52:
            case 53:
                reportError("setters/getters not yet implemented", ast);
                break;
            case 89:
            case 87:
                reportError("try/throw/catch/finally not yet implemented", ast);
                break;
            case 64:
            case 68:
            case 44:
                reportError("break/continure/labels not yet implemented", ast);
                break;
            case 93:
            case 58:
                reportError("generators/yield not yet implemented", ast);
                break;
            case 45:
                reportError("for .. in loops not yet implemented", ast);
                break;
            case 57:
            case 59:
                reportError("array comprehensions not yet implemented", ast);
                break;
            case 82:
                for(var idx = 0; idx < ast.children.length; idx++) {
                    infer(ast.children[idx].children[1], memVars, ins, outs);
                }
                break;
            case 47:
            case 50:
                for(var idx = 0; idx < ast.children.length; idx++) {
                    infer(ast.children[idx].children[1], memVars, ins, outs);
                }
                break;
            case 95:
                reportError("general objects not yet implemented", ast);
                break;
            case 81:
            case 56:
                reportError("let not yet implemented", ast);
                break;
            case 85:
                reportError("switch not yet implemented", ast);
                break;

            // unsupported binary functions
            case 80:
                reportError("instanceof not yet implemented", ast);
                break;
            case 13:
            case 14:
                reportError("non-strict equality not yet implemented", ast);
                break;
            case 79:
                reportError("in not yet implemented", ast);
                break;

            // unsupported literals
            case 83:
                reportError("null not yet implemented", ast);
                break;
            case 63:
                reportError("regular expressions not yet implemented", ast);
                break;
            case 62:
                reportError("strings not yet implemented", ast);
                break;

            case 69:  // whatever this is...
            default:
                throw "unhandled node type in analysis: " + ast.type;
        }

    };

    function doInfer (ast) {
        if (ast.type !== 77) {
            reportBug("you probaly wanted to call the inference on a function node!");
        } else {
            infer(ast.body);
        }
    };

    return {
        "infer" : doInfer
    };
}();
