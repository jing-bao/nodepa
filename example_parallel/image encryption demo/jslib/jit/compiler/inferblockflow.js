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


//
// This phase collects flow information of variables, namely
//
// in: all the variables that _may_ be read _before_ they are locally defined
// locals: those variables that are _known_ to be locally defined
// outs: those variables that _may_ flow out of the context
//
// The corresponding information is attached to all block nodes and
// to do/for/while loop nodes.
//
RiverTrail.InferBlockFlow = function () {
    var definitions = Narcissus.definitions;
    eval(definitions.consts);
    eval(RiverTrail.definitions.consts);

    const debug = false;

    //
    // error reporting
    //
    var reportError = RiverTrail.Helper.reportError;
    var reportBug = RiverTrail.Helper.reportBug;

    var findSelectionRoot = RiverTrail.Helper.findSelectionRoot;

    // set for remembering identifiers
    var IdSet = function () {
        this._store = {};
        this._store.__proto__ = null;
        return this;
    }
    var ISP = IdSet.prototype = {};
    ISP.subtract = function subtract (other) {
        if (typeof(other) === 'string') {
            delete this._store[other];
        } else {
            for (var name in other._store) {
                delete this._store[name];
            }
        }
        return this;
    };
    ISP.union = function union (other) {
        if (typeof(other) === 'string') {
            this._store[other] = null;
        } else {
            for (var name in other._store) {
                this._store[name] = null;
            }
        }
        return this;
    };
    ISP.clone = function clone () {
        var result = new IdSet();
        result.union(this);
        return result;
    };
    ISP.contains = function contains (name) {
        return (this._store[name] !== undefined);
    };
    ISP.intersect = function intersect (other) {
        if (typeof(other) === 'string') {
            this.subtract(this).union(other);
        } else {
            for (var name in this._store) {
                if (!other.contains(name)) {
                    delete this._store[name];
                }
            }
        }
        return this;
    };
    ISP.toString = function toString () {
        return "{" + Object.keys(this._store).join(",") + "}";
    };

    function infer(ast, ins, outs, locals) {
        "use strict";

        if ((ins === undefined)) {
            (ast.type === 77) || reportBug("you probably wanted to start inference with a function!");
            ins = new IdSet();
            outs = new IdSet();
            locals = new IdSet();
            infer(ast.body, ins, outs, locals);
        }
        switch (ast.type) {
            case 42:
                ast.funDecls.forEach(function (f) {infer(f);});
                // fallthrough
            case 43:
                var blockIns = new IdSet();
                var blockOuts = new IdSet();
                var blockLocals = new IdSet();
                ast.children.forEach(function (ast) { infer(ast, blockIns, blockOuts, blockLocals); });
                ast.ins = blockIns.clone();
                ast.outs = blockOuts;
                ast.locals = blockLocals;
                // add all block in vars that are not locally defined in the outer scope to the outer in vars
                ins.union(blockIns.subtract(locals));
                // add our outs and locals to the global outs and locals
                outs.union(blockOuts);
                locals.union(blockLocals);
                break;

            //
            // statements
            //
            case 77:
                // this is not an applied occurence but the declaration, so we do not do anything here
                break;
            case 84:
                infer(ast.value, ins, outs, locals);
                break;
            //
            // loops
            //
            case 72:
                var doIns = new IdSet();
                var doOuts = new IdSet();
                var doLocals = new IdSet();
                infer(ast.body, doIns, doOuts, doLocals);
                infer(ast.condition, doIns, doOuts, doLocals);
                ast.ins = doIns.clone();
                ast.outs = doOuts;
                ast.locals = doLocals;
                // join things up. For the ins, we have to take the global locals out!
                ins.union(doIns.subtract(locals));
                // outs just become joined
                outs.union(doOuts);
                // as do locals, as the loop is always executed at least once
                locals.union(doLocals);
                break;
            case 76:
            case 94:
                if (ast.setup) {
                    infer(ast.setup, ins, outs, locals);
                }
                // the loop body may not be executed, so we have to consider the union of the ins/outs
                // of the loop with the ins/outs when not executing that path
                var loopIns = new IdSet();
                var loopOuts = new IdSet();
                var loopLocals = new IdSet();
                infer(ast.condition, loopIns, loopOuts, loopLocals);
                // the conditional is executed once more later on, so we remeber its ins and outs
                var condIns = loopIns.clone();
                var condOuts = loopOuts.clone();
                var condLocals = loopLocals.clone();
                // now do the body
                infer(ast.body, loopIns, loopOuts, loopLocals);
                if (ast.update) {
                    infer(ast.update, loopIns, loopOuts, loopLocals);
                }
                ast.ins = loopIns.clone();
                ast.outs = loopOuts;
                ast.locals = loopLocals;
                // now we have the in/out knowledge when executing the loop. Join with outer in/outs
                // we drop the locals here, as we do not know whether this path is actually executed!
                ins.union(loopIns.subtract(locals));
                outs.union(loopOuts);
                // the condition is always executed once at least, so we have to take its ins/outs into account
                ins.union(condIns.subtract(locals));
                outs.union(condOuts);
                locals.union(condLocals);
                break;
            case 78:
                infer(ast.condition, ins, outs, locals);
                var thenIns = new IdSet();
                var thenOuts = new IdSet();
                var thenLocals = new IdSet();
                infer(ast.thenPart, thenIns, thenOuts, thenLocals);
                ins.union(thenIns.subtract(locals));
                outs.union(thenOuts);
                if (ast.elsePart) {
                    var elseIns = new IdSet()
                    var elseOuts = new IdSet()
                    var elseLocals = new IdSet()
                    infer(ast.elsePart, elseIns, elseOuts, elseLocals);
                    ins.union(elseIns.subtract(locals));
                    outs.union(elseOuts);
                    thenLocals.intersect(elseLocals);
                }
                locals.union(thenLocals);
                break;
            case 2:
                if (ast.expression) {
                    infer(ast.expression, ins, outs, locals);
                }
                break;
            case 91:
            case 67:
                ast.children.forEach(function (ast) {
                                         if (ast.initializer) {
                                             infer(ast.initializer, ins, outs, locals);
                                             locals.union(ast.value);
                                             outs.union(ast.value);
                                         }
                                     });
                break;
            case 4:
                // children[0] is the left hand side, children[1] is the right hand side.
                // both can be expressions. 
                switch (ast.children[0].type) {
                    case 60:
                        // simple case of a = expr
                        infer(ast.children[1], ins, outs, locals);
                        locals.union(ast.children[0].value);
                        outs.union(ast.children[0].value);
                        if (ast.assignOp) {
                            // this is a id <op>= expr, so we have an in dependency, too
                            ins.union(ast.children[0].value);
                        }
                        break;
                    case 48:
                        infer(ast.children[0], ins, outs, locals);
                        infer(ast.children[1], ins, outs, locals);
                        // a[expr_i] = expr
                        // today, a needs to be a nested selection. We walked through it once, which tags
                        // it as an IN. Additionally, it now becomes an out. It, however, does not
                        // become a local, as it is not _fully_ locally defined!
                        outs.union(findSelectionRoot(ast.children[0]).value);
                        break;
                    case 35:
                        // not allowed for now as object cannot be mutated :-)
                        // we should never get here.
                        //reportBug("objects may not be mutated!");
                        infer(ast.children[0], ins, outs, locals);
                        infer(ast.children[1], ins, outs, locals);
                        break;
                    default:
                        reportBug("unhandled lhs in assignment");
                        break;
                }
                break;
                
            // 
            // expressions
            //
            case 3:
                // left to right we go...
                ast.children.forEach(function (ast) { infer(ast, ins, outs, locals); });
                break;
            case 5:
                // Same logic as used for conditionals
                infer(ast.children[0], ins, outs, locals);
                var elseIns = ins.clone();
                var elseOuts = outs.clone();
                var elseLocals = locals.clone();
                var thenLocals = locals.clone();
                infer(ast.children[1], ins, outs, thenLocals);
                infer(ast.children[2], elseIns, elseOuts, elseLocals);
                ins.union(elseIns);
                outs.union(elseOuts);
                thenLocals.intersect(elseLocals);
                locals.union(thenLocals);
                break;
                
            // side effecting expressions
            case 33:
            case 34:
                ins.union(ast.value);
                locals.union(ast.value); // only if expr is not an array!
                outs.union(ast.value);
                break;

            // n-ary expressions
            case 24: 
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
                //fallthrough;
                // misc other stuff that just requires a map
                ast.children.forEach(function (val) { infer(val, ins, outs, locals); });
                break;
            case 46:
                if(ast.children[0].type === 35 &&
                        (ast.children[0].children[0].value === "RiverTrailUtils" ||
                        ast.children[0].children[1].value === "createArray")) {
                    infer(ast.children[1].children[1], ins, outs, locals);
                    break;
                }
                ast.children.forEach(function (val) { infer(val, ins, outs, locals); });
                break;
            case 55:      
            case 96:
            case 97:
            case 98:
            case 49:
            case 48:
                ast.children.forEach(function (val) { infer(val, ins, outs, locals); });
                break;

            // dot is special: only look at child number one, number two is a label!
            case 35:
                infer(ast.children[0], ins, outs, locals);
                break;

            // literals
            case 60:
            case 86:
                if (!locals.contains(ast.value)) {
                    ins.union(ast.value);
                }
                break;


            case 61:
            case 88:
            case 74:
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
            case 47:
                reportError("general objects not yet implemented", ast);
                break;
            case 50:
                for(var idx = 0; idx < ast.children.length; idx++) {
                    infer(ast.children[idx].children[1], ins, outs, locals);
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
    }

    return {
        "infer" : infer,
    };
}();
